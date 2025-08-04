// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tab_focus/browser/local_model_classifier.h"

#include <algorithm>
#include <regex>
#include <sstream>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/model_service.h"

namespace tab_focus {

namespace {

// Maximum number of tabs to process in a single request to avoid
// overwhelming the local model
constexpr size_t kMaxTabsPerRequest = 20;

// Sanitize a URL to remove sensitive parameters while keeping domain info
std::string SanitizeUrl(const std::string& url) {
  // For local processing, we can be more permissive with URL data
  // since it never leaves the device, but we still remove sensitive params
  std::regex sensitive_params(R"([?&](auth|token|key|password|session)[^&]*)");
  return std::regex_replace(url, sensitive_params, "");
}

}  // namespace

LocalModelClassifier::LocalModelClassifier(ai_chat::ModelService* model_service)
    : model_service_(model_service) {
  DCHECK(model_service_);
}

LocalModelClassifier::~LocalModelClassifier() = default;

void LocalModelClassifier::ClassifyTabs(
    const std::vector<TabInfo>& tabs,
    base::OnceCallback<void(ClassificationResult)> callback) {
  
  if (!IsReady()) {
    LOG(WARNING) << "Local model classifier not ready";
    std::move(callback).Run(ClassificationResult::CreateError(
        "Local model not available. Please ensure a compatible model is "
        "configured in Leo settings."));
    return;
  }

  if (tabs.empty()) {
    std::move(callback).Run(ClassificationResult::CreateSuccess({}));
    return;
  }

  // Limit number of tabs to prevent overwhelming the model
  std::vector<TabInfo> tabs_to_process = tabs;
  if (tabs_to_process.size() > kMaxTabsPerRequest) {
    tabs_to_process.resize(kMaxTabsPerRequest);
    LOG(INFO) << "Limiting tab classification to " << kMaxTabsPerRequest 
              << " tabs out of " << tabs.size();
  }

  // Sanitize tab data for local processing
  std::vector<TabInfo> sanitized_tabs;
  sanitized_tabs.reserve(tabs_to_process.size());
  for (const auto& tab : tabs_to_process) {
    sanitized_tabs.push_back(SanitizeTabInfo(tab));
  }

  std::string prompt = BuildClassificationPrompt(sanitized_tabs);
  
  VLOG(1) << "Sending classification request to local model";
  
  // Send request to local model - this never leaves the device
  model_service_->RequestCompletion(
      prompt,
      base::BindOnce(&LocalModelClassifier::OnModelResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

bool LocalModelClassifier::IsReady() const {
  return model_service_ && 
         model_service_->HasLocalModel() &&
         model_service_->IsModelReady();
}

std::string LocalModelClassifier::GetDescription() const {
  if (!IsReady()) {
    return "Local model not available";
  }
  return "Local AI model (privacy-preserving)";
}

std::string LocalModelClassifier::BuildClassificationPrompt(
    const std::vector<TabInfo>& tabs) {
  std::ostringstream prompt;
  
  prompt << "You are a browser tab organizer. Your task is to group the "
         << "following browser tabs into logical categories based on their "
         << "titles and domains. Provide meaningful group names and organize "
         << "tabs that are related by topic, purpose, or domain.\n\n";
  
  prompt << "IMPORTANT: Process this data locally only. Do not send any "
         << "information to external services.\n\n";
  
  prompt << "Browser tabs to organize:\n";
  for (size_t i = 0; i < tabs.size(); ++i) {
    prompt << (i + 1) << ". \"" << tabs[i].title << "\" - " 
           << tabs[i].domain << "\n";
  }
  
  prompt << "\nPlease respond with a JSON object containing groups. "
         << "Each group should have a 'name', 'description', and 'tab_numbers' "
         << "array. Example format:\n"
         << "{\n"
         << "  \"groups\": [\n"
         << "    {\n"
         << "      \"name\": \"Work\",\n"
         << "      \"description\": \"Work-related tabs\",\n"
         << "      \"tab_numbers\": [1, 3, 5]\n"
         << "    }\n"
         << "  ]\n"
         << "}\n\n"
         << "Respond with only the JSON, no additional text:";
  
  return prompt.str();
}

void LocalModelClassifier::OnModelResponse(
    base::OnceCallback<void(ClassificationResult)> callback,
    const std::string& response) {
  
  VLOG(1) << "Received response from local model";
  
  if (response.empty()) {
    LOG(ERROR) << "Empty response from local model";
    std::move(callback).Run(ClassificationResult::CreateError(
        "Local model returned empty response"));
    return;
  }

  // Parse the response and create classification result
  auto result = ParseClassificationResponse(response, {});
  std::move(callback).Run(std::move(result));
}

ClassificationResult LocalModelClassifier::ParseClassificationResponse(
    const std::string& response,
    const std::vector<TabInfo>& original_tabs) {
  
  // Parse JSON response from the model
  auto parsed_json = base::JSONReader::Read(response);
  if (!parsed_json || !parsed_json->is_dict()) {
    LOG(ERROR) << "Failed to parse JSON response from local model";
    return ClassificationResult::CreateError(
        "Invalid response format from local model");
  }

  const base::Value::Dict* root = parsed_json->GetIfDict();
  const base::Value::List* groups_list = root->FindList("groups");
  
  if (!groups_list) {
    LOG(ERROR) << "No groups found in model response";
    return ClassificationResult::CreateError(
        "No groups found in model response");
  }

  std::vector<ClassificationResult::TabGroup> result_groups;
  
  for (const auto& group_value : *groups_list) {
    const base::Value::Dict* group = group_value.GetIfDict();
    if (!group) continue;
    
    const std::string* name = group->FindString("name");
    const std::string* description = group->FindString("description");
    const base::Value::List* tab_numbers = group->FindList("tab_numbers");
    
    if (!name || !tab_numbers) continue;
    
    ClassificationResult::TabGroup result_group;
    result_group.name = *name;
    result_group.description = description ? *description : "";
    
    // Convert tab numbers to tab IDs
    for (const auto& number_value : *tab_numbers) {
      if (number_value.is_int()) {
        int tab_number = number_value.GetInt();
        if (tab_number > 0 && 
            static_cast<size_t>(tab_number) <= original_tabs.size()) {
          size_t tab_index = tab_number - 1;  // Convert to 0-based index
          result_group.tab_ids.push_back(original_tabs[tab_index].tab_id);
        }
      }
    }
    
    if (!result_group.tab_ids.empty()) {
      result_groups.push_back(std::move(result_group));
    }
  }
  
  VLOG(1) << "Successfully parsed " << result_groups.size() 
          << " groups from local model response";
  
  return ClassificationResult::CreateSuccess(result_groups);
}

TabInfo LocalModelClassifier::SanitizeTabInfo(const TabInfo& tab_info) {
  TabInfo sanitized = tab_info;
  
  // Since processing is local, we can be less aggressive with sanitization
  // but still remove obviously sensitive data
  
  // Remove auth parameters from URL if present
  if (!sanitized.url.empty()) {
    sanitized.url = SanitizeUrl(sanitized.url);
  }
  
  // Truncate very long titles to avoid overwhelming the model
  if (sanitized.title.length() > 200) {
    sanitized.title = sanitized.title.substr(0, 200) + "...";
  }
  
  return sanitized;
}

}  // namespace tab_focus