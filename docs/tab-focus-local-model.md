# Tab Focus Mode Local Model Implementation

## Overview

This document outlines the implementation plan for adding a privacy-preserving "Local Model" option to Tab Focus Mode. When enabled, this option ensures all tab grouping/classification happens locally using self-hosted or system models, without sending any tab data to Brave servers.

## Background

Tab Focus Mode is implemented in brave-core and currently:
1. Collects tab titles and origins from open tabs
2. Sends this data to Brave's Leo servers for AI-powered classification
3. Groups tabs based on the server response
4. Provides organization suggestions to users

The privacy-preserving option will leverage Brave's existing BYOM (Bring Your Own Model) infrastructure used by Leo Assistant.

## Requirements

1. **User-facing setting**: Add a toggle to enable "Local Model" classification
2. **Data privacy**: No tab data leaves the device when local mode is enabled
3. **BYOM integration**: Leverage existing self-hosted/local model infrastructure
4. **Clear separation**: Distinct code paths for local vs remote classification
5. **Settings UI**: Surface the option with clear privacy benefits explanation
6. **Documentation**: Help reviewers understand the implementation
7. **Testing**: Verify local-only processing when enabled

## Architecture

### Core Components

```
components/
├── ai_chat/                          # Existing Leo/BYOM infrastructure
│   ├── core/browser/engine/          # Model management
│   └── core/browser/model_service/   # Local model service
└── tab_focus/                        # Tab Focus Mode implementation
    ├── browser/
    │   ├── tab_focus_service.h       # Main service interface
    │   ├── tab_classifier.h          # Classification logic
    │   └── local_model_classifier.h  # New local classifier
    └── common/
        └── pref_names.h              # Preference definitions
```

### Settings Integration

```
browser/ui/webui/settings/
├── brave_settings_ui.cc              # Settings registration
└── settings/leo-assistant/           # Leo settings page
    └── customization/                # BYOM configuration
```

## Implementation Plan

### Phase 1: Core Infrastructure

#### 1.1 Preference Definition

```cpp
// components/tab_focus/common/pref_names.h
namespace tab_focus {
namespace prefs {

// Controls whether Tab Focus Mode uses local model for classification
inline constexpr char kUseLocalModel[] = "brave.tab_focus.use_local_model";

}  // namespace prefs
}  // namespace tab_focus
```

#### 1.2 Service Interface Enhancement

```cpp
// components/tab_focus/browser/tab_focus_service.h
class TabFocusService {
 public:
  enum class ClassificationMode {
    kRemote,  // Use Brave servers (default)
    kLocal    // Use local/self-hosted model
  };

  // Set classification mode
  void SetClassificationMode(ClassificationMode mode);
  
  // Get current mode
  ClassificationMode GetClassificationMode() const;

 private:
  std::unique_ptr<TabClassifier> CreateClassifier(ClassificationMode mode);
  
  ClassificationMode classification_mode_ = ClassificationMode::kRemote;
  std::unique_ptr<TabClassifier> classifier_;
};
```

#### 1.3 Classifier Abstraction

```cpp
// components/tab_focus/browser/tab_classifier.h
class TabClassifier {
 public:
  virtual ~TabClassifier() = default;
  
  // Classify tabs into groups
  virtual void ClassifyTabs(
      const std::vector<TabInfo>& tabs,
      base::OnceCallback<void(ClassificationResult)> callback) = 0;
  
  // Check if classifier is ready to use
  virtual bool IsReady() const = 0;
};

// Remote classifier (existing implementation)
class RemoteTabClassifier : public TabClassifier {
  // Implementation sends data to Brave servers
};

// Local classifier (new implementation)
class LocalTabClassifier : public TabClassifier {
 public:
  explicit LocalTabClassifier(ai_chat::ModelService* model_service);
  
  void ClassifyTabs(
      const std::vector<TabInfo>& tabs,
      base::OnceCallback<void(ClassificationResult)> callback) override;
  
  bool IsReady() const override;
  
 private:
  raw_ptr<ai_chat::ModelService> model_service_;
  // Local processing logic
};
```

### Phase 2: Settings UI Integration

#### 2.1 Leo Settings Page Enhancement

```typescript
// components/ai_chat/resources/page/settings/customization.tsx
interface TabFocusSettings {
  useLocalModel: boolean;
}

function TabFocusSection() {
  const [settings, setSettings] = useState<TabFocusSettings>({
    useLocalModel: false
  });

  return (
    <SettingsSection title="Tab Focus Mode">
      <SettingsToggle
        title="Use Local Model for Tab Classification"
        subtitle="When enabled, tab titles and origins stay on your device and are processed using your local or self-hosted model instead of Brave's servers."
        checked={settings.useLocalModel}
        onChange={(enabled) => {
          setSettings({...settings, useLocalModel: enabled});
          // Update preference
        }}
      />
      <PrivacyBenefits visible={settings.useLocalModel} />
    </SettingsSection>
  );
}

function PrivacyBenefits({ visible }: { visible: boolean }) {
  if (!visible) return null;
  
  return (
    <div className="privacy-benefits">
      <h4>Privacy Benefits:</h4>
      <ul>
        <li>Tab data never leaves your device</li>
        <li>No tracking or data collection by Brave</li>
        <li>Full control over your browsing patterns</li>
        <li>Compatible with self-hosted AI models</li>
      </ul>
    </div>
  );
}
```

#### 2.2 Settings Registration

```cpp
// browser/ui/webui/settings/brave_settings_ui.cc
void BraveSettingsUI::UpdateSettingsPageUIHandler(
    const std::string& page_name,
    content::WebUIDataSource* source) {
  
  if (page_name == "leo-assistant") {
    // Add Tab Focus Mode local model preference
    source->AddBoolean("tabFocusUseLocalModel",
        profile_->GetPrefs()->GetBoolean(
            tab_focus::prefs::kUseLocalModel));
  }
}
```

### Phase 3: Local Model Integration

#### 3.1 Model Service Integration

```cpp
// components/tab_focus/browser/local_model_classifier.cc
LocalTabClassifier::LocalTabClassifier(ai_chat::ModelService* model_service)
    : model_service_(model_service) {}

void LocalTabClassifier::ClassifyTabs(
    const std::vector<TabInfo>& tabs,
    base::OnceCallback<void(ClassificationResult)> callback) {
  
  if (!IsReady()) {
    std::move(callback).Run(ClassificationResult::CreateError(
        "Local model not available"));
    return;
  }

  // Build classification prompt for local model
  std::string prompt = BuildClassificationPrompt(tabs);
  
  // Use local model for classification (no network request)
  model_service_->RequestCompletion(
      prompt,
      base::BindOnce(&LocalTabClassifier::OnClassificationResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

bool LocalTabClassifier::IsReady() const {
  return model_service_ && 
         model_service_->HasLocalModel() &&
         model_service_->IsModelReady();
}

std::string LocalTabClassifier::BuildClassificationPrompt(
    const std::vector<TabInfo>& tabs) {
  // Create privacy-aware prompt that only uses titles/origins
  // No sensitive content or tracking data included
  std::string prompt = 
      "Classify the following browser tabs into logical groups based on their "
      "titles and domains. Provide group names and tab assignments:\n\n";
  
  for (size_t i = 0; i < tabs.size(); ++i) {
    prompt += base::StringPrintf("Tab %zu: %s (%s)\n", 
                                 i + 1,
                                 tabs[i].title.c_str(),
                                 tabs[i].domain.c_str());
  }
  
  prompt += "\nPlease group these tabs and suggest meaningful group names.";
  return prompt;
}
```

### Phase 4: Testing and Validation

#### 4.1 Unit Tests

```cpp
// components/tab_focus/browser/local_model_classifier_unittest.cc
class LocalTabClassifierTest : public testing::Test {
 protected:
  void SetUp() override {
    model_service_ = std::make_unique<ai_chat::MockModelService>();
    classifier_ = std::make_unique<LocalTabClassifier>(model_service_.get());
  }

  std::unique_ptr<ai_chat::MockModelService> model_service_;
  std::unique_ptr<LocalTabClassifier> classifier_;
};

TEST_F(LocalTabClassifierTest, RequiresLocalModel) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillOnce(testing::Return(false));
  
  EXPECT_FALSE(classifier_->IsReady());
}

TEST_F(LocalTabClassifierTest, ClassifiesTabsLocally) {
  EXPECT_CALL(*model_service_, HasLocalModel())
      .WillRepeatedly(testing::Return(true));
  EXPECT_CALL(*model_service_, IsModelReady())
      .WillRepeatedly(testing::Return(true));
  
  std::vector<TabInfo> tabs = CreateTestTabs();
  bool callback_called = false;
  
  classifier_->ClassifyTabs(
      tabs,
      base::BindOnce([&](ClassificationResult result) {
        EXPECT_TRUE(result.IsSuccess());
        callback_called = true;
      }));
  
  EXPECT_TRUE(callback_called);
  
  // Verify no network requests were made
  EXPECT_EQ(0, GetNetworkRequestCount());
}
```

#### 4.2 Privacy Verification Test

```cpp
// Test to ensure no data leaves the device in local mode
TEST_F(TabFocusServiceTest, LocalModePreservesPrivacy) {
  // Enable local mode
  pref_service_->SetBoolean(tab_focus::prefs::kUseLocalModel, true);
  
  // Set up network monitoring
  auto network_monitor = std::make_unique<NetworkRequestMonitor>();
  
  // Perform tab classification
  service_->ClassifyCurrentTabs(base::BindOnce([](auto result) {
    // Classification should succeed
    EXPECT_TRUE(result.IsSuccess());
  }));
  
  // Verify no network requests to Brave servers
  EXPECT_EQ(0, network_monitor->GetRequestCount("*.brave.com"));
  EXPECT_EQ(0, network_monitor->GetRequestCount("*leo*"));
}
```

## Privacy and Security Considerations

### Data Handling

1. **Local Processing**: When local mode is enabled, tab titles and origins are processed entirely on-device
2. **No Telemetry**: No usage statistics or model performance data is sent to Brave
3. **Model Security**: Leverage existing BYOM security measures for local model validation
4. **Prompt Sanitization**: Ensure classification prompts don't leak sensitive information

### User Transparency

1. **Clear Settings**: Obvious toggle with explanation of privacy benefits
2. **Status Indication**: UI shows whether local or remote mode is active
3. **Model Requirements**: Clear indication when local model is not available
4. **Fallback Behavior**: Graceful degradation when local model fails

## Migration and Rollout

### Gradual Rollout

1. **Feature Flag**: Initially hidden behind brave://flags for testing
2. **Limited Release**: Enable for subset of users with BYOM models
3. **Full Release**: Make available to all users with local models
4. **Default Behavior**: Keep remote mode as default for compatibility

### User Education

1. **Help Documentation**: Explain benefits and requirements of local mode
2. **Setup Guidance**: Instructions for configuring compatible local models
3. **Performance Expectations**: Set realistic expectations for local processing speed

## Future Enhancements

1. **Model Optimization**: Specialized lightweight models for tab classification
2. **Offline Support**: Classification works without internet connection
3. **Advanced Privacy**: Option to exclude certain domains from classification
4. **Custom Prompts**: Allow users to customize classification criteria

## Conclusion

This implementation provides a privacy-preserving alternative to the current Tab Focus Mode while leveraging Brave's existing BYOM infrastructure. The design ensures clear separation between local and remote processing paths, maintaining the existing user experience while adding enhanced privacy options for users who prefer local processing.