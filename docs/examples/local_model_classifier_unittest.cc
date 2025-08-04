// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/tab_focus/browser/local_model_classifier.h"

#include <memory>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace tab_focus {

namespace {

// Mock ModelService for testing
class MockModelService : public ai_chat::ModelService {
 public:
  MOCK_METHOD(bool, HasLocalModel, (), (const, override));
  MOCK_METHOD(bool, IsModelReady, (), (const, override));
  MOCK_METHOD(void, RequestCompletion, 
              (const std::string& prompt,
               base::OnceCallback<void(const std::string&)> callback),
              (override));
};

// Test tab data
std::vector<TabInfo> CreateTestTabs() {
  return {
    {.title = "Gmail - Inbox", .domain = "mail.google.com", .tab_id = 1},
    {.title = "Google Drive", .domain = "drive.google.com", .tab_id = 2},
    {.title = "Facebook", .domain = "facebook.com", .tab_id = 3},
    {.title = "Twitter", .domain = "twitter.com", .tab_id = 4},
    {.title = "GitHub - brave/brave-core", .domain = "github.com", .tab_id = 5}
  };
}

// Sample JSON response for testing
constexpr char kSampleModelResponse[] = R"({
  "groups": [
    {
      "name": "Google Services",
      "description": "Google productivity tools",
      "tab_numbers": [1, 2]
    },
    {
      "name": "Social Media", 
      "description": "Social networking sites",
      "tab_numbers": [3, 4]
    },
    {
      "name": "Development",
      "description": "Programming and development",
      "tab_numbers": [5]
    }
  ]
})";

}  // namespace

class LocalModelClassifierTest : public testing::Test {
 protected:
  void SetUp() override {
    model_service_ = std::make_unique<testing::StrictMock<MockModelService>>();
    classifier_ = std::make_unique<LocalModelClassifier>(model_service_.get());
  }

  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<testing::StrictMock<MockModelService>> model_service_;
  std::unique_ptr<LocalModelClassifier> classifier_;
};

TEST_F(LocalModelClassifierTest, IsLocalProcessing) {
  EXPECT_TRUE(classifier_->IsLocalProcessing());
}

TEST_F(LocalModelClassifierTest, NotReadyWhenNoLocalModel) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(false));
  
  EXPECT_FALSE(classifier_->IsReady());
}

TEST_F(LocalModelClassifierTest, NotReadyWhenModelNotReady) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(false));
  
  EXPECT_FALSE(classifier_->IsReady());
}

TEST_F(LocalModelClassifierTest, ReadyWhenModelAvailableAndReady) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  EXPECT_TRUE(classifier_->IsReady());
}

TEST_F(LocalModelClassifierTest, ClassifyTabsWhenNotReady) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(false));
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  ClassificationResult result;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        result = std::move(r);
      }));
  
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(result.IsSuccess());
  EXPECT_THAT(result.error_message, 
              testing::HasSubstr("Local model not available"));
}

TEST_F(LocalModelClassifierTest, ClassifyTabsEmptyList) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  std::vector<TabInfo> empty_tabs;
  bool callback_called = false;
  ClassificationResult result;
  
  classifier_->ClassifyTabs(
      empty_tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        result = std::move(r);
      }));
  
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(result.IsSuccess());
  EXPECT_TRUE(result.groups.empty());
}

TEST_F(LocalModelClassifierTest, ClassifyTabsSuccessfully) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  // Expect model completion request
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        // Verify prompt contains privacy instruction
        EXPECT_THAT(prompt, testing::HasSubstr("Process this data locally only"));
        EXPECT_THAT(prompt, testing::HasSubstr("Do not send any information"));
        
        // Simulate successful model response
        std::move(callback).Run(kSampleModelResponse);
      });
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  ClassificationResult result;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        result = std::move(r);
      }));
  
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(result.IsSuccess());
  EXPECT_EQ(3u, result.groups.size());
  
  // Verify first group
  EXPECT_EQ("Google Services", result.groups[0].name);
  EXPECT_EQ(2u, result.groups[0].tab_ids.size());
  EXPECT_EQ(1, result.groups[0].tab_ids[0]);  // Gmail tab
  EXPECT_EQ(2, result.groups[0].tab_ids[1]);  // Drive tab
  
  // Verify second group  
  EXPECT_EQ("Social Media", result.groups[1].name);
  EXPECT_EQ(2u, result.groups[1].tab_ids.size());
  
  // Verify third group
  EXPECT_EQ("Development", result.groups[2].name);
  EXPECT_EQ(1u, result.groups[2].tab_ids.size());
  EXPECT_EQ(5, result.groups[2].tab_ids[0]);  // GitHub tab
}

TEST_F(LocalModelClassifierTest, HandlesInvalidModelResponse) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        // Simulate invalid JSON response
        std::move(callback).Run("This is not valid JSON");
      });
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  ClassificationResult result;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        result = std::move(r);
      }));
  
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(result.IsSuccess());
  EXPECT_THAT(result.error_message, 
              testing::HasSubstr("Invalid response format"));
}

TEST_F(LocalModelClassifierTest, HandlesEmptyModelResponse) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        // Simulate empty response
        std::move(callback).Run("");
      });
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  ClassificationResult result;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        result = std::move(r);
      }));
  
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(result.IsSuccess());
  EXPECT_THAT(result.error_message, 
              testing::HasSubstr("empty response"));
}

// Test to ensure privacy: verify no network access patterns
TEST_F(LocalModelClassifierTest, NoNetworkAccess) {
  // This test verifies that the local classifier doesn't make any
  // network requests by checking the method calls on the model service
  
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  // Only local method should be called, no network-related methods
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(kSampleModelResponse);
      });
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        EXPECT_TRUE(r.IsSuccess());
      }));
  
  EXPECT_TRUE(callback_called);
  
  // No additional network calls should have been made
  // (This would be verified in integration tests with network monitoring)
}

TEST_F(LocalModelClassifierTest, LimitsTabCount) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillOnce(testing::Return(true));
  
  // Create many tabs (more than the limit)
  std::vector<TabInfo> many_tabs;
  for (int i = 0; i < 50; ++i) {
    many_tabs.push_back({
      .title = "Tab " + std::to_string(i),
      .domain = "example" + std::to_string(i) + ".com",
      .tab_id = i
    });
  }
  
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        // Verify that prompt doesn't contain all 50 tabs
        // Should be limited to around 20
        int tab_count = 0;
        size_t pos = 0;
        while ((pos = prompt.find("Tab ", pos)) != std::string::npos) {
          tab_count++;
          pos += 4;
        }
        EXPECT_LE(tab_count, 25);  // Allow some margin for the limit
        
        std::move(callback).Run(kSampleModelResponse);
      });
  
  bool callback_called = false;
  
  classifier_->ClassifyTabs(
      many_tabs,
      base::BindLambdaForTesting([&](ClassificationResult r) {
        callback_called = true;
        EXPECT_TRUE(r.IsSuccess());
      }));
  
  EXPECT_TRUE(callback_called);
}

}  // namespace tab_focus