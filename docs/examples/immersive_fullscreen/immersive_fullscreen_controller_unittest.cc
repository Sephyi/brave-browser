// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/immersive_fullscreen/immersive_fullscreen_controller.h"

#include "base/test/task_environment.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget.h"

#include "brave/browser/ui/immersive_fullscreen/pref_names.h"

namespace immersive_fullscreen {

namespace {

// Mock observer for testing immersive fullscreen events
class MockImmersiveFullscreenObserver : public ImmersiveFullscreenObserver {
 public:
  MOCK_METHOD(void, OnImmersiveUIVisibilityChanged, (bool visible), (override));
  MOCK_METHOD(void, OnImmersiveModeToggled, (bool enabled), (override));
  MOCK_METHOD(void, OnMouseHoverStateChanged, (bool hovering_top_edge), (override));
};

// Mock browser view for testing
class MockBrowserView : public BrowserView {
 public:
  MockBrowserView() = default;
  ~MockBrowserView() override = default;

  MOCK_METHOD(views::View*, toolbar, (), (const, override));
  MOCK_METHOD(views::View*, tabstrip, (), (const, override));
  MOCK_METHOD(views::View*, bookmark_bar, (), (const, override));
  MOCK_METHOD(views::View*, download_shelf, (), (const, override));
  MOCK_METHOD(views::View*, infobar_container, (), (const, override));
  MOCK_METHOD(views::Widget*, GetWidget, (), (const, override));
};

// Mock UI view for testing visibility changes
class MockUIView : public views::View {
 public:
  MOCK_METHOD(void, SetVisible, (bool visible), (override));
  MOCK_METHOD(bool, GetVisible, (), (const, override));
};

}  // namespace

class ImmersiveFullscreenControllerTest : public views::ViewsTestBase {
 public:
  void SetUp() override {
    ViewsTestBase::SetUp();
    
    // Set up test preferences
    prefs_.registry()->RegisterBooleanPref(prefs::kImmersiveFullscreenEnabled, true);
    prefs_.registry()->RegisterIntegerPref(prefs::kAutoHideDelayMs, 2000);
    prefs_.registry()->RegisterIntegerPref(prefs::kTopEdgeSensitivityPx, 5);
    prefs_.registry()->RegisterIntegerPref(prefs::kAnimationDurationMs, 300);
    prefs_.registry()->RegisterBooleanPref(prefs::kShowOnAddressBarFocus, true);
    prefs_.registry()->RegisterBooleanPref(prefs::kShowOnKeyboardActivity, false);
    prefs_.registry()->RegisterBooleanPref(prefs::kHasSeenIntroduction, false);
    prefs_.registry()->RegisterBooleanPref(prefs::kRespectLegacyFullscreenPref, true);
    
    // Set up mock browser view
    browser_view_ = std::make_unique<MockBrowserView>();
    
    // Set up mock UI views
    toolbar_view_ = std::make_unique<MockUIView>();
    tab_strip_view_ = std::make_unique<MockUIView>();
    bookmark_bar_view_ = std::make_unique<MockUIView>();
    
    // Configure mock expectations
    ON_CALL(*browser_view_, toolbar())
        .WillByDefault(testing::Return(toolbar_view_.get()));
    ON_CALL(*browser_view_, tabstrip())
        .WillByDefault(testing::Return(tab_strip_view_.get()));
    ON_CALL(*browser_view_, bookmark_bar())
        .WillByDefault(testing::Return(bookmark_bar_view_.get()));
    
    // Create mock widget
    widget_ = std::make_unique<views::Widget>();
    views::Widget::InitParams params = CreateParams(views::Widget::InitParams::TYPE_WINDOW);
    params.bounds = gfx::Rect(0, 0, 1200, 800);
    widget_->Init(std::move(params));
    
    ON_CALL(*browser_view_, GetWidget())
        .WillByDefault(testing::Return(widget_.get()));
    
    // Create controller
    controller_ = std::make_unique<ImmersiveFullscreenController>(
        browser_view_.get(), &prefs_);
    
    // Set up test-friendly timing
    controller_->SetAnimationDurationForTesting(base::Milliseconds(1));
    controller_->SetAutoHideDelayForTesting(base::Milliseconds(10));
  }

  void TearDown() override {
    controller_.reset();
    widget_.reset();
    browser_view_.reset();
    toolbar_view_.reset();
    tab_strip_view_.reset();
    bookmark_bar_view_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<MockBrowserView> browser_view_;
  std::unique_ptr<MockUIView> toolbar_view_;
  std::unique_ptr<MockUIView> tab_strip_view_;
  std::unique_ptr<MockUIView> bookmark_bar_view_;
  std::unique_ptr<views::Widget> widget_;
  std::unique_ptr<ImmersiveFullscreenController> controller_;
  MockImmersiveFullscreenObserver observer_;
};

TEST_F(ImmersiveFullscreenControllerTest, InitialStateIsDisabled) {
  EXPECT_FALSE(controller_->IsEnabled());
  EXPECT_EQ(ImmersiveFullscreenController::State::kDisabled, controller_->GetState());
  EXPECT_FALSE(controller_->IsUIVisible());
}

TEST_F(ImmersiveFullscreenControllerTest, EnableImmersiveMode) {
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveModeToggled(true));
  EXPECT_CALL(*toolbar_view_, SetVisible(false));
  EXPECT_CALL(*tab_strip_view_, SetVisible(false));
  EXPECT_CALL(*bookmark_bar_view_, SetVisible(false));
  
  controller_->SetEnabled(true);
  
  EXPECT_TRUE(controller_->IsEnabled());
  EXPECT_EQ(ImmersiveFullscreenController::State::kEnabled, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, DisableImmersiveMode) {
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveModeToggled(false));
  EXPECT_CALL(*toolbar_view_, SetVisible(true));
  EXPECT_CALL(*tab_strip_view_, SetVisible(true));
  EXPECT_CALL(*bookmark_bar_view_, SetVisible(true));
  
  controller_->SetEnabled(false);
  
  EXPECT_FALSE(controller_->IsEnabled());
  EXPECT_EQ(ImmersiveFullscreenController::State::kDisabled, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, RevealUIOnMouseHover) {
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnMouseHoverStateChanged(true));
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(true));
  EXPECT_CALL(*toolbar_view_, SetVisible(true));
  EXPECT_CALL(*tab_strip_view_, SetVisible(true));
  EXPECT_CALL(*bookmark_bar_view_, SetVisible(true));
  
  // Mouse move to top edge (within 5px sensitivity)
  gfx::Point top_edge_point(600, 2);
  controller_->OnMouseMoved(top_edge_point);
  
  EXPECT_TRUE(controller_->IsUIVisible());
  EXPECT_EQ(ImmersiveFullscreenController::State::kRevealed, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, HideUIWhenMouseLeavesTopEdge) {
  controller_->SetEnabled(true);
  
  // First reveal UI with mouse hover
  controller_->OnMouseMoved(gfx::Point(600, 2));
  EXPECT_TRUE(controller_->IsUIVisible());
  
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnMouseHoverStateChanged(false));
  
  // Move mouse away from top edge
  controller_->OnMouseMoved(gfx::Point(600, 100));
  
  // Manually trigger auto-hide timer for testing
  controller_->TriggerAutoHideTimerForTesting();
  
  EXPECT_FALSE(controller_->IsUIVisible());
  EXPECT_EQ(ImmersiveFullscreenController::State::kEnabled, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, RevealUIOnAddressBarFocus) {
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(true));
  
  controller_->OnAddressBarFocused();
  
  EXPECT_TRUE(controller_->IsUIVisible());
  EXPECT_EQ(ImmersiveFullscreenController::State::kRevealed, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, KeepUIVisibleWhileAddressBarFocused) {
  controller_->SetEnabled(true);
  controller_->OnAddressBarFocused();
  
  // Try to hide UI while address bar is focused - should not hide
  controller_->HideUIImmediately();
  
  EXPECT_TRUE(controller_->IsUIVisible());
}

TEST_F(ImmersiveFullscreenControllerTest, HideUIAfterAddressBarBlur) {
  controller_->SetEnabled(true);
  controller_->OnAddressBarFocused();
  
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(false));
  
  controller_->OnAddressBarBlurred();
  
  // Manually trigger auto-hide timer
  controller_->TriggerAutoHideTimerForTesting();
  
  EXPECT_FALSE(controller_->IsUIVisible());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, PinAndUnpinUI) {
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(true));
  
  controller_->PinUI();
  
  EXPECT_TRUE(controller_->IsUIVisible());
  EXPECT_EQ(ImmersiveFullscreenController::State::kPinned, controller_->GetState());
  
  // Try to hide pinned UI - should not hide
  controller_->HideUIImmediately();
  EXPECT_TRUE(controller_->IsUIVisible());
  
  // Unpin and verify auto-hide works
  controller_->UnpinUI();
  EXPECT_EQ(ImmersiveFullscreenController::State::kRevealed, controller_->GetState());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, FullscreenStateChanges) {
  // Test enabling immersive mode on fullscreen entry
  controller_->OnFullscreenStateChanged(true);
  EXPECT_TRUE(controller_->IsEnabled());
  
  // Test disabling immersive mode on fullscreen exit
  controller_->OnFullscreenStateChanged(false);
  EXPECT_FALSE(controller_->IsEnabled());
}

TEST_F(ImmersiveFullscreenControllerTest, KeyboardActivityReveal) {
  // Enable keyboard activity triggering
  prefs_.SetBoolean(prefs::kShowOnKeyboardActivity, true);
  controller_ = std::make_unique<ImmersiveFullscreenController>(
      browser_view_.get(), &prefs_);
  controller_->SetAnimationDurationForTesting(base::Milliseconds(1));
  
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(true));
  
  controller_->OnKeyboardActivity();
  
  EXPECT_TRUE(controller_->IsUIVisible());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, RespectLegacyFullscreenPreference) {
  // Set legacy preference to always show toolbar
  prefs_.registry()->RegisterBooleanPref("show_fullscreen_toolbar", true);
  prefs_.SetBoolean("show_fullscreen_toolbar", true);
  
  // Try to enable immersive mode - should be blocked
  controller_->SetEnabled(true);
  EXPECT_FALSE(controller_->IsEnabled());
}

TEST_F(ImmersiveFullscreenControllerTest, MouseSensitivityConfiguration) {
  controller_->SetEnabled(true);
  
  // Test with default 5px sensitivity
  controller_->OnMouseMoved(gfx::Point(600, 4)); // Within sensitivity
  EXPECT_TRUE(controller_->IsUIVisible());
  
  controller_->OnMouseMoved(gfx::Point(600, 100)); // Outside sensitivity
  controller_->TriggerAutoHideTimerForTesting();
  EXPECT_FALSE(controller_->IsUIVisible());
  
  // Change sensitivity to 10px
  prefs_.SetInteger(prefs::kTopEdgeSensitivityPx, 10);
  controller_ = std::make_unique<ImmersiveFullscreenController>(
      browser_view_.get(), &prefs_);
  controller_->SetEnabled(true);
  
  controller_->OnMouseMoved(gfx::Point(600, 8)); // Within new sensitivity
  EXPECT_TRUE(controller_->IsUIVisible());
}

TEST_F(ImmersiveFullscreenControllerTest, ActiveTabChangeShowsUI) {
  controller_->SetEnabled(true);
  controller_->AddObserver(&observer_);
  
  EXPECT_CALL(observer_, OnImmersiveUIVisibilityChanged(true));
  
  controller_->OnActiveTabChanged(nullptr);
  
  EXPECT_TRUE(controller_->IsUIVisible());
  
  controller_->RemoveObserver(&observer_);
}

TEST_F(ImmersiveFullscreenControllerTest, IntroductionTrackingPreference) {
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kHasSeenIntroduction));
  
  controller_->SetEnabled(true);
  
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kHasSeenIntroduction));
}

TEST_F(ImmersiveFullscreenControllerTest, ObserverManagement) {
  MockImmersiveFullscreenObserver observer1;
  MockImmersiveFullscreenObserver observer2;
  
  controller_->AddObserver(&observer1);
  controller_->AddObserver(&observer2);
  
  EXPECT_CALL(observer1, OnImmersiveModeToggled(true));
  EXPECT_CALL(observer2, OnImmersiveModeToggled(true));
  
  controller_->SetEnabled(true);
  
  // Remove one observer
  controller_->RemoveObserver(&observer1);
  
  EXPECT_CALL(observer1, OnImmersiveModeToggled(false)).Times(0);
  EXPECT_CALL(observer2, OnImmersiveModeToggled(false));
  
  controller_->SetEnabled(false);
  
  controller_->RemoveObserver(&observer2);
}

// Performance test for rapid mouse movements
TEST_F(ImmersiveFullscreenControllerTest, RapidMouseMovements) {
  controller_->SetEnabled(true);
  
  // Simulate rapid mouse movements
  for (int i = 0; i < 100; ++i) {
    gfx::Point point(600, i % 20); // Alternate between top edge and below
    controller_->OnMouseMoved(point);
  }
  
  // Should still function correctly after rapid movements
  EXPECT_TRUE(controller_->IsEnabled());
}

// Edge case test for mouse at exact boundary
TEST_F(ImmersiveFullscreenControllerTest, MouseAtExactBoundary) {
  controller_->SetEnabled(true);
  
  // Test mouse at exact sensitivity boundary (5px)
  controller_->OnMouseMoved(gfx::Point(600, 5));
  EXPECT_FALSE(controller_->IsUIVisible());
  
  // Test mouse just inside boundary (4px)
  controller_->OnMouseMoved(gfx::Point(600, 4));
  EXPECT_TRUE(controller_->IsUIVisible());
}

}  // namespace immersive_fullscreen