// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_
#define BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/widget/widget_observer.h"

class BrowserView;
class PrefService;

namespace content {
class WebContents;
}

namespace views {
class Widget;
}

namespace immersive_fullscreen {

// Observer interface for immersive fullscreen state changes
class ImmersiveFullscreenObserver {
 public:
  virtual ~ImmersiveFullscreenObserver() = default;
  
  // Called when UI visibility changes in immersive mode
  virtual void OnImmersiveUIVisibilityChanged(bool visible) {}
  
  // Called when immersive mode is enabled or disabled
  virtual void OnImmersiveModeToggled(bool enabled) {}
  
  // Called when mouse hover state changes
  virtual void OnMouseHoverStateChanged(bool hovering_top_edge) {}
};

// Controls immersive fullscreen behavior for browser UI elements
// Manages auto-hiding of toolbars, tabs, and bookmarks bar in fullscreen mode
// with smooth animations and smart reveal on mouse hover/focus
class ImmersiveFullscreenController : public views::WidgetObserver {
 public:
  // UI elements that can be controlled in immersive mode
  enum class UIElement {
    kToolbar,           // Address bar and navigation buttons
    kTabStrip,          // Tab bar
    kBookmarksBar,      // Bookmarks toolbar
    kDownloadShelf,     // Download shelf
    kInfoBarContainer,  // Information bars
  };

  // Current state of immersive fullscreen
  enum class State {
    kDisabled,          // Not in fullscreen or immersive mode disabled
    kEnabled,           // In immersive mode with UI hidden
    kRevealed,          // In immersive mode with UI temporarily shown
    kPinned,            // In immersive mode with UI pinned visible
  };

  explicit ImmersiveFullscreenController(BrowserView* browser_view,
                                         PrefService* pref_service);
  ~ImmersiveFullscreenController() override;

  // Disallow copy and assign
  ImmersiveFullscreenController(const ImmersiveFullscreenController&) = delete;
  ImmersiveFullscreenController& operator=(const ImmersiveFullscreenController&) = delete;

  // Enable or disable immersive fullscreen mode
  void SetEnabled(bool enabled);
  
  // Check if immersive mode is currently enabled
  bool IsEnabled() const { return state_ != State::kDisabled; }
  
  // Check if UI is currently visible
  bool IsUIVisible() const { 
    return state_ == State::kRevealed || state_ == State::kPinned; 
  }
  
  // Get current immersive state
  State GetState() const { return state_; }

  // Temporarily reveal UI (e.g., on mouse hover or focus)
  void RevealUI();
  
  // Hide UI after delay (if not pinned)
  void HideUIAfterDelay();
  
  // Immediately hide UI (if not pinned)
  void HideUIImmediately();
  
  // Pin UI visible until explicitly unpinned
  void PinUI();
  
  // Unpin UI (will auto-hide after delay if no interaction)
  void UnpinUI();

  // Handle mouse movement for top-edge detection
  void OnMouseMoved(const gfx::Point& location_in_screen);
  
  // Handle focus events
  void OnAddressBarFocused();
  void OnAddressBarBlurred();
  
  // Handle keyboard activity
  void OnKeyboardActivity();

  // Called when fullscreen state changes
  void OnFullscreenStateChanged(bool is_fullscreen);
  
  // Called when tab changes
  void OnActiveTabChanged(content::WebContents* web_contents);

  // Observer management
  void AddObserver(ImmersiveFullscreenObserver* observer);
  void RemoveObserver(ImmersiveFullscreenObserver* observer);

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // Testing helpers
  void SetAnimationDurationForTesting(base::TimeDelta duration);
  void SetAutoHideDelayForTesting(base::TimeDelta delay);
  void TriggerAutoHideTimerForTesting();

 private:
  // Animation state for UI transitions
  enum class AnimationState {
    kNone,
    kShowingUI,
    kHidingUI,
  };

  // Initialize immersive mode settings from preferences
  void InitializeFromPrefs();
  
  // Update immersive mode based on current fullscreen state and preferences
  void UpdateImmersiveMode();
  
  // Start animation to show UI elements
  void AnimateShowUI();
  
  // Start animation to hide UI elements
  void AnimateHideUI();
  
  // Complete current animation and set final state
  void CompleteAnimation();
  
  // Check if mouse is in top edge sensitive area
  bool IsMouseInTopEdge(const gfx::Point& location_in_screen) const;
  
  // Get screen bounds for the browser window
  gfx::Rect GetBrowserScreenBounds() const;
  
  // Schedule auto-hide timer
  void ScheduleAutoHide();
  
  // Cancel auto-hide timer
  void CancelAutoHide();
  
  // Auto-hide callback
  void OnAutoHideTimer();
  
  // Notify observers of state changes
  void NotifyVisibilityChanged(bool visible);
  void NotifyModeToggled(bool enabled);
  void NotifyMouseHoverChanged(bool hovering);

  // Check if immersive mode should be disabled due to preferences
  bool ShouldDisableImmersiveMode() const;
  
  // Get UI element views
  views::View* GetUIElementView(UIElement element) const;
  
  // Update visibility of specific UI element
  void SetUIElementVisible(UIElement element, bool visible);

  // Browser view that owns this controller
  BrowserView* browser_view_;
  
  // Preference service for settings
  PrefService* pref_service_;
  
  // Current immersive state
  State state_ = State::kDisabled;
  
  // Current animation state
  AnimationState animation_state_ = AnimationState::kNone;
  
  // Whether UI should be shown (based on mouse/focus state)
  bool should_show_ui_ = false;
  
  // Whether address bar currently has focus
  bool address_bar_focused_ = false;
  
  // Whether mouse is hovering in top edge area
  bool mouse_in_top_edge_ = false;
  
  // Timer for auto-hiding UI
  base::OneShotTimer auto_hide_timer_;
  
  // Configuration from preferences
  base::TimeDelta auto_hide_delay_;
  base::TimeDelta animation_duration_;
  int top_edge_sensitivity_px_ = 5;
  bool show_on_address_bar_focus_ = true;
  bool show_on_keyboard_activity_ = false;
  
  // Observer list
  base::ObserverList<ImmersiveFullscreenObserver> observers_;
  
  // Weak pointer factory
  base::WeakPtrFactory<ImmersiveFullscreenController> weak_ptr_factory_{this};
};

}  // namespace immersive_fullscreen

#endif  // BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_IMMERSIVE_FULLSCREEN_CONTROLLER_H_