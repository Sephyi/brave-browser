// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_TAB_CLASSIFIER_H_
#define BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_TAB_CLASSIFIER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

namespace tab_focus {

// Represents information about a browser tab for classification
struct TabInfo {
  std::string title;
  std::string domain;
  std::string url;  // Only used for local processing, never sent to servers
  int tab_id;
};

// Result of tab classification operation
struct ClassificationResult {
  enum class Status {
    kSuccess,
    kError,
    kModelNotReady
  };
  
  struct TabGroup {
    std::string name;
    std::vector<int> tab_ids;
    std::string description;
  };
  
  Status status = Status::kError;
  std::vector<TabGroup> groups;
  std::string error_message;
  
  static ClassificationResult CreateSuccess(
      const std::vector<TabGroup>& groups) {
    ClassificationResult result;
    result.status = Status::kSuccess;
    result.groups = groups;
    return result;
  }
  
  static ClassificationResult CreateError(const std::string& message) {
    ClassificationResult result;
    result.status = Status::kError;
    result.error_message = message;
    return result;
  }
  
  bool IsSuccess() const { return status == Status::kSuccess; }
};

// Abstract base class for tab classification implementations
class TabClassifier {
 public:
  virtual ~TabClassifier() = default;
  
  // Classify tabs into logical groups
  // This method must not send any data outside the device when implementing
  // local classification
  virtual void ClassifyTabs(
      const std::vector<TabInfo>& tabs,
      base::OnceCallback<void(ClassificationResult)> callback) = 0;
  
  // Check if classifier is ready to use
  virtual bool IsReady() const = 0;
  
  // Returns true if this classifier processes data locally
  virtual bool IsLocalProcessing() const = 0;
  
  // Get human-readable description for UI
  virtual std::string GetDescription() const = 0;
};

}  // namespace tab_focus

#endif  // BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_TAB_CLASSIFIER_H_