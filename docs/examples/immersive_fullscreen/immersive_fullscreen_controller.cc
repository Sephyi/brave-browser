// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/immersive_fullscreen/immersive_fullscreen_controller.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/download/download_shelf_view.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/animation/animation_delegate_views.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

#include "brave/browser/ui/immersive_fullscreen/pref_names.h"

namespace immersive_fullscreen {

namespace {

// Default values for immersive fullscreen behavior
constexpr base::TimeDelta kDefaultAutoHideDelay = base::Milliseconds(2000);
constexpr base::TimeDelta kDefaultAnimationDuration = base::Milliseconds(300);
constexpr int kDefaultTopEdgeSensitivity = 5;

// Animation delegate for smooth UI transitions
class ImmersiveAnimationDelegate : public views::AnimationDelegateViews {
 public:
  explicit ImmersiveAnimationDelegate(
      base::WeakPtr<ImmersiveFullscreenController> controller)
      : controller_(controller) {}

  ~ImmersiveAnimationDelegate() override = default;

  // views::AnimationDelegateViews:
  void AnimationProgressed(const gfx::Animation* animation) override {
    if (controller_) {
      controller_->OnAnimationProgressed(animation->GetCurrentValue());
    }
  }

  void AnimationEnded(const gfx::Animation* animation) override {
    if (controller_) {
      controller_->OnAnimationCompleted();
    }
  }

 private:
  base::WeakPtr<ImmersiveFullscreenController> controller_;
};

}  // namespace

ImmersiveFullscreenController::ImmersiveFullscreenController(
    BrowserView* browser_view,
    PrefService* pref_service)
    : browser_view_(browser_view),
      pref_service_(pref_service),
      animation_(std::make_unique<gfx::SlideAnimation>(this)) {
  
  DCHECK(browser_view_);
  DCHECK(pref_service_);
  
  InitializeFromPrefs();
  
  // Set up animation
  animation_->SetSlideDuration(animation_duration_);
  animation_->SetTweenType(gfx::Tween::EASE_IN_OUT);
  
  // Observe widget destruction
  browser_view_->GetWidget()->AddObserver(this);
}

ImmersiveFullscreenController::~ImmersiveFullscreenController() {
  if (browser_view_ && browser_view_->GetWidget()) {
    browser_view_->GetWidget()->RemoveObserver(this);
  }
}

void ImmersiveFullscreenController::SetEnabled(bool enabled) {
  if (enabled == IsEnabled()) {
    return;
  }

  if (enabled && ShouldDisableImmersiveMode()) {
    // Don't enable if preferences indicate it should be disabled
    return;
  }

  State old_state = state_;
  
  if (enabled) {
    state_ = State::kEnabled;
    
    // Record that user has seen immersive mode
    if (!pref_service_->GetBoolean(prefs::kHasSeenIntroduction)) {
      pref_service_->SetBoolean(prefs::kHasSeenIntroduction, true);
    }
    
    // Start with UI hidden unless there's a reason to show it
    if (should_show_ui_ || address_bar_focused_) {
      RevealUI();
    } else {
      AnimateHideUI();
    }
  } else {
    state_ = State::kDisabled;
    
    // Cancel any pending animations or timers
    CancelAutoHide();
    CompleteAnimation();
    
    // Make sure all UI is visible when disabling
    SetAllUIElementsVisible(true);
  }

  if (old_state != state_) {
    NotifyModeToggled(enabled);
    
    // Record usage metrics
    UMA_HISTOGRAM_BOOLEAN("Brave.ImmersiveFullscreen.Enabled", enabled);
  }
}

void ImmersiveFullscreenController::RevealUI() {
  if (state_ == State::kDisabled) {
    return;
  }

  bool was_visible = IsUIVisible();
  
  if (state_ == State::kEnabled) {
    state_ = State::kRevealed;
  }
  
  CancelAutoHide();
  AnimateShowUI();
  
  if (!was_visible) {
    NotifyVisibilityChanged(true);
  }
}

void ImmersiveFullscreenController::HideUIAfterDelay() {
  if (state_ == State::kDisabled || state_ == State::kPinned) {
    return;
  }

  if (should_show_ui_ || address_bar_focused_) {
    // Don't hide if there's a reason to keep UI visible
    return;
  }

  ScheduleAutoHide();
}

void ImmersiveFullscreenController::HideUIImmediately() {
  if (state_ == State::kDisabled || state_ == State::kPinned) {
    return;
  }

  CancelAutoHide();
  
  bool was_visible = IsUIVisible();
  state_ = State::kEnabled;
  
  AnimateHideUI();
  
  if (was_visible) {
    NotifyVisibilityChanged(false);
  }
}

void ImmersiveFullscreenController::PinUI() {
  if (state_ == State::kDisabled) {
    return;
  }

  CancelAutoHide();
  
  bool was_visible = IsUIVisible();
  state_ = State::kPinned;
  
  if (!was_visible) {
    AnimateShowUI();
    NotifyVisibilityChanged(true);
  }
}

void ImmersiveFullscreenController::UnpinUI() {
  if (state_ != State::kPinned) {
    return;
  }

  state_ = State::kRevealed;
  
  // Auto-hide after delay if no reason to keep visible
  if (!should_show_ui_ && !address_bar_focused_) {
    HideUIAfterDelay();
  }
}

void ImmersiveFullscreenController::OnMouseMoved(
    const gfx::Point& location_in_screen) {
  if (state_ == State::kDisabled) {
    return;
  }

  bool was_in_top_edge = mouse_in_top_edge_;
  mouse_in_top_edge_ = IsMouseInTopEdge(location_in_screen);
  
  if (mouse_in_top_edge_ != was_in_top_edge) {
    NotifyMouseHoverChanged(mouse_in_top_edge_);
  }

  bool old_should_show = should_show_ui_;
  should_show_ui_ = mouse_in_top_edge_;
  
  if (should_show_ui_ && !old_should_show) {
    // Mouse entered top edge - reveal UI
    RevealUI();
  } else if (!should_show_ui_ && old_should_show && !address_bar_focused_) {
    // Mouse left top edge and address bar not focused - hide after delay
    HideUIAfterDelay();
  }
}

void ImmersiveFullscreenController::OnAddressBarFocused() {
  address_bar_focused_ = true;
  
  if (show_on_address_bar_focus_ && state_ != State::kDisabled) {
    RevealUI();
  }
}

void ImmersiveFullscreenController::OnAddressBarBlurred() {
  address_bar_focused_ = false;
  
  if (state_ != State::kDisabled && state_ != State::kPinned && 
      !should_show_ui_) {
    HideUIAfterDelay();
  }
}

void ImmersiveFullscreenController::OnKeyboardActivity() {
  if (show_on_keyboard_activity_ && state_ != State::kDisabled) {
    RevealUI();
  }
}

void ImmersiveFullscreenController::OnFullscreenStateChanged(bool is_fullscreen) {
  if (is_fullscreen) {
    // Enable immersive mode when entering fullscreen (if preference allows)
    if (pref_service_->GetBoolean(prefs::kImmersiveFullscreenEnabled)) {
      SetEnabled(true);
    }
  } else {
    // Disable immersive mode when exiting fullscreen
    SetEnabled(false);
  }
}

void ImmersiveFullscreenController::OnActiveTabChanged(
    content::WebContents* web_contents) {
  // Reset state when switching tabs
  if (state_ != State::kDisabled) {
    // Briefly show UI when switching tabs
    RevealUI();
    if (state_ != State::kPinned) {
      HideUIAfterDelay();
    }
  }
}

void ImmersiveFullscreenController::AddObserver(
    ImmersiveFullscreenObserver* observer) {
  observers_.AddObserver(observer);
}

void ImmersiveFullscreenController::RemoveObserver(
    ImmersiveFullscreenObserver* observer) {
  observers_.RemoveObserver(observer);
}

void ImmersiveFullscreenController::OnWidgetDestroying(views::Widget* widget) {
  widget->RemoveObserver(this);
}

void ImmersiveFullscreenController::SetAnimationDurationForTesting(
    base::TimeDelta duration) {
  animation_duration_ = duration;
  animation_->SetSlideDuration(duration);
}

void ImmersiveFullscreenController::SetAutoHideDelayForTesting(
    base::TimeDelta delay) {
  auto_hide_delay_ = delay;
}

void ImmersiveFullscreenController::TriggerAutoHideTimerForTesting() {
  OnAutoHideTimer();
}

// Private methods

void ImmersiveFullscreenController::InitializeFromPrefs() {
  auto_hide_delay_ = base::Milliseconds(
      pref_service_->GetInteger(prefs::kAutoHideDelayMs));
  if (auto_hide_delay_.is_zero()) {
    auto_hide_delay_ = kDefaultAutoHideDelay;
  }

  animation_duration_ = base::Milliseconds(
      pref_service_->GetInteger(prefs::kAnimationDurationMs));
  if (animation_duration_.is_zero()) {
    animation_duration_ = kDefaultAnimationDuration;
  }

  top_edge_sensitivity_px_ = pref_service_->GetInteger(prefs::kTopEdgeSensitivityPx);
  if (top_edge_sensitivity_px_ <= 0) {
    top_edge_sensitivity_px_ = kDefaultTopEdgeSensitivity;
  }

  show_on_address_bar_focus_ = pref_service_->GetBoolean(prefs::kShowOnAddressBarFocus);
  show_on_keyboard_activity_ = pref_service_->GetBoolean(prefs::kShowOnKeyboardActivity);
}

void ImmersiveFullscreenController::AnimateShowUI() {
  if (animation_state_ == AnimationState::kShowingUI) {
    return;
  }

  animation_state_ = AnimationState::kShowingUI;
  
  if (animation_->IsShowing()) {
    // Already showing, just make sure it completes
    return;
  }

  animation_->Show();
}

void ImmersiveFullscreenController::AnimateHideUI() {
  if (animation_state_ == AnimationState::kHidingUI) {
    return;
  }

  animation_state_ = AnimationState::kHidingUI;
  
  if (!animation_->IsShowing()) {
    // Already hidden, just make sure it completes
    return;
  }

  animation_->Hide();
}

void ImmersiveFullscreenController::CompleteAnimation() {
  if (animation_state_ == AnimationState::kNone) {
    return;
  }

  animation_->End();
  animation_state_ = AnimationState::kNone;
}

bool ImmersiveFullscreenController::IsMouseInTopEdge(
    const gfx::Point& location_in_screen) const {
  gfx::Rect browser_bounds = GetBrowserScreenBounds();
  
  return location_in_screen.x() >= browser_bounds.x() &&
         location_in_screen.x() < browser_bounds.right() &&
         location_in_screen.y() >= browser_bounds.y() &&
         location_in_screen.y() < (browser_bounds.y() + top_edge_sensitivity_px_);
}

gfx::Rect ImmersiveFullscreenController::GetBrowserScreenBounds() const {
  return browser_view_->GetWidget()->GetWindowBoundsInScreen();
}

void ImmersiveFullscreenController::ScheduleAutoHide() {
  auto_hide_timer_.Start(
      FROM_HERE,
      auto_hide_delay_,
      base::BindOnce(&ImmersiveFullscreenController::OnAutoHideTimer,
                     base::Unretained(this)));
}

void ImmersiveFullscreenController::CancelAutoHide() {
  auto_hide_timer_.Stop();
}

void ImmersiveFullscreenController::OnAutoHideTimer() {
  HideUIImmediately();
}

void ImmersiveFullscreenController::NotifyVisibilityChanged(bool visible) {
  for (ImmersiveFullscreenObserver& observer : observers_) {
    observer.OnImmersiveUIVisibilityChanged(visible);
  }
}

void ImmersiveFullscreenController::NotifyModeToggled(bool enabled) {
  for (ImmersiveFullscreenObserver& observer : observers_) {
    observer.OnImmersiveModeToggled(enabled);
  }
}

void ImmersiveFullscreenController::NotifyMouseHoverChanged(bool hovering) {
  for (ImmersiveFullscreenObserver& observer : observers_) {
    observer.OnMouseHoverStateChanged(hovering);
  }
}

bool ImmersiveFullscreenController::ShouldDisableImmersiveMode() const {
  // Respect legacy "Always Show Toolbar in Full Screen" preference
  if (pref_service_->GetBoolean(prefs::kRespectLegacyFullscreenPref)) {
    if (pref_service_->GetBoolean(::prefs::kShowFullscreenToolbar)) {
      return true;
    }
  }

  return false;
}

views::View* ImmersiveFullscreenController::GetUIElementView(UIElement element) const {
  switch (element) {
    case UIElement::kToolbar:
      return browser_view_->toolbar();
    case UIElement::kTabStrip:
      return browser_view_->tabstrip();
    case UIElement::kBookmarksBar:
      return browser_view_->bookmark_bar();
    case UIElement::kDownloadShelf:
      return browser_view_->download_shelf();
    case UIElement::kInfoBarContainer:
      return browser_view_->infobar_container();
  }
  return nullptr;
}

void ImmersiveFullscreenController::SetUIElementVisible(UIElement element, bool visible) {
  views::View* view = GetUIElementView(element);
  if (view) {
    view->SetVisible(visible);
  }
}

void ImmersiveFullscreenController::SetAllUIElementsVisible(bool visible) {
  SetUIElementVisible(UIElement::kToolbar, visible);
  SetUIElementVisible(UIElement::kTabStrip, visible);
  SetUIElementVisible(UIElement::kBookmarksBar, visible);
  // Don't control download shelf and infobar visibility - they have their own logic
}

void ImmersiveFullscreenController::OnAnimationProgressed(double value) {
  // Apply progressive visibility based on animation value
  // value = 0.0 means fully hidden, value = 1.0 means fully visible
  
  // For now, use simple show/hide - could be enhanced with partial visibility
  bool should_be_visible = value > 0.5;
  
  SetUIElementVisible(UIElement::kToolbar, should_be_visible);
  SetUIElementVisible(UIElement::kTabStrip, should_be_visible);
  SetUIElementVisible(UIElement::kBookmarksBar, should_be_visible);
}

void ImmersiveFullscreenController::OnAnimationCompleted() {
  animation_state_ = AnimationState::kNone;
  
  // Ensure final state is correct
  bool should_be_visible = (state_ == State::kRevealed || state_ == State::kPinned);
  SetAllUIElementsVisible(should_be_visible);
}

}  // namespace immersive_fullscreen