// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_PREF_NAMES_H_
#define BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_PREF_NAMES_H_

namespace immersive_fullscreen {
namespace prefs {

// Controls whether immersive fullscreen mode is enabled
// When enabled, UI elements auto-hide in fullscreen and reappear on mouse hover
// Type: Boolean
// Default: true (enable immersive experience by default)
inline constexpr char kImmersiveFullscreenEnabled[] = 
    "brave.immersive_fullscreen.enabled";

// Controls the delay before auto-hiding UI elements in milliseconds
// Type: Integer
// Default: 2000 (2 seconds)
inline constexpr char kAutoHideDelayMs[] = 
    "brave.immersive_fullscreen.auto_hide_delay_ms";

// Controls the mouse sensitivity area at the top of the screen (in pixels)
// Type: Integer  
// Default: 5 (5 pixel area at top triggers UI reveal)
inline constexpr char kTopEdgeSensitivityPx[] = 
    "brave.immersive_fullscreen.top_edge_sensitivity_px";

// Controls animation duration for show/hide transitions in milliseconds
// Type: Integer
// Default: 300 (300ms smooth transition)
inline constexpr char kAnimationDurationMs[] = 
    "brave.immersive_fullscreen.animation_duration_ms";

// Controls whether to show UI when address bar gains focus
// Type: Boolean
// Default: true (always show UI when typing in address bar)
inline constexpr char kShowOnAddressBarFocus[] = 
    "brave.immersive_fullscreen.show_on_address_bar_focus";

// Controls whether to show UI on any keyboard activity
// Type: Boolean
// Default: false (only show on specific focus events)
inline constexpr char kShowOnKeyboardActivity[] = 
    "brave.immersive_fullscreen.show_on_keyboard_activity";

// Stores whether user has seen the immersive mode introduction
// Type: Boolean
// Default: false (show introduction on first fullscreen)
inline constexpr char kHasSeenIntroduction[] = 
    "brave.immersive_fullscreen.has_seen_introduction";

// Controls whether to disable immersive mode when "Always Show Toolbar in Full Screen" is enabled
// Type: Boolean
// Default: true (respect legacy fullscreen preference)
inline constexpr char kRespectLegacyFullscreenPref[] = 
    "brave.immersive_fullscreen.respect_legacy_fullscreen_pref";

}  // namespace prefs
}  // namespace immersive_fullscreen

#endif  // BRAVE_BROWSER_UI_IMMERSIVE_FULLSCREEN_PREF_NAMES_H_