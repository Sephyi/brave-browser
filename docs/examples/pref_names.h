// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TAB_FOCUS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_TAB_FOCUS_COMMON_PREF_NAMES_H_

namespace tab_focus {
namespace prefs {

// Controls whether Tab Focus Mode uses local model for classification
// instead of sending data to Brave servers.
// Type: Boolean
// Default: false (use remote classification for compatibility)
inline constexpr char kUseLocalModel[] = "brave.tab_focus.use_local_model";

// Stores the last known state of local model availability
// Used to show appropriate UI messages when local model is not ready
// Type: Boolean
// Default: false
inline constexpr char kLocalModelAvailable[] = 
    "brave.tab_focus.local_model_available";

// Controls whether to show privacy notification about local processing
// Type: Boolean  
// Default: true (show notification first time local mode is enabled)
inline constexpr char kShowLocalModeNotification[] = 
    "brave.tab_focus.show_local_mode_notification";

// Maximum number of tabs to process in local mode to avoid overwhelming
// local models
// Type: Integer
// Default: 20
inline constexpr char kMaxTabsForLocalProcessing[] = 
    "brave.tab_focus.max_tabs_local_processing";

}  // namespace prefs
}  // namespace tab_focus

#endif  // BRAVE_COMPONENTS_TAB_FOCUS_COMMON_PREF_NAMES_H_