// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_LOCAL_MODEL_CLASSIFIER_H_
#define BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_LOCAL_MODEL_CLASSIFIER_H_

#include <memory>
#include <string>

#include "brave/components/tab_focus/browser/tab_classifier.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"

namespace ai_chat {
class ModelService;
}

namespace tab_focus {

// Local tab classifier that uses on-device AI models for privacy-preserving
// tab classification. This implementation ensures no tab data leaves the
// device by leveraging Brave's BYOM (Bring Your Own Model) infrastructure.
class LocalModelClassifier : public TabClassifier {
 public:
  explicit LocalModelClassifier(ai_chat::ModelService* model_service);
  ~LocalModelClassifier() override;

  // TabClassifier implementation
  void ClassifyTabs(
      const std::vector<TabInfo>& tabs,
      base::OnceCallback<void(ClassificationResult)> callback) override;
  
  bool IsReady() const override;
  bool IsLocalProcessing() const override { return true; }
  std::string GetDescription() const override;

 private:
  // Build a privacy-aware classification prompt
  std::string BuildClassificationPrompt(const std::vector<TabInfo>& tabs);
  
  // Process the model's response into classification result
  void OnModelResponse(
      base::OnceCallback<void(ClassificationResult)> callback,
      const std::string& response);
  
  // Parse the model's text response into structured groups
  ClassificationResult ParseClassificationResponse(
      const std::string& response,
      const std::vector<TabInfo>& original_tabs);
  
  // Sanitize tab information to remove sensitive data before processing
  TabInfo SanitizeTabInfo(const TabInfo& tab_info);
  
  raw_ptr<ai_chat::ModelService> model_service_;
  base::WeakPtrFactory<LocalModelClassifier> weak_ptr_factory_{this};
};

}  // namespace tab_focus

#endif  // BRAVE_COMPONENTS_TAB_FOCUS_BROWSER_LOCAL_MODEL_CLASSIFIER_H_