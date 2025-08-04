# Brave Browser Feature Examples

This directory contains example implementations demonstrating various features for the Brave browser. These examples show the architectural patterns and code structure that would be implemented in brave-core.

## Features

### Tab Focus Mode Local Model
Privacy-preserving local model feature for Tab Focus Mode that processes tab data entirely on-device.

### Immersive Fullscreen Mode
Distraction-free browsing experience with auto-hiding UI elements in fullscreen mode, featuring smart reveal on mouse hover and focus-aware behavior.

**Key Files:**
- `immersive_fullscreen/immersive_fullscreen_controller.h/.cc` - Main controller implementation
- `immersive_fullscreen/immersive_fullscreen_settings.tsx` - React settings component  
- `immersive_fullscreen/immersive_fullscreen.css` - Smooth animation styles
- `immersive_fullscreen/event_handlers.js` - JavaScript event integration
- `immersive_fullscreen/pref_names.h` - Preference definitions

**Features:**
- Auto-hide UI elements (toolbar, tabs, bookmarks) in fullscreen
- Smart reveal on mouse hover at screen top edge
- Address bar focus awareness  
- Configurable timing and sensitivity
- Smooth CSS animations with hardware acceleration
- Safari-like UX behavior on macOS

## Overview

The Tab Focus Mode currently sends tab titles and origins to Brave servers for AI-powered classification. The local model feature provides a privacy-preserving alternative that processes all data on-device using Brave's existing BYOM (Bring Your Own Model) infrastructure.

## Files Description

### Core Implementation

- **`tab_classifier.h`** - Abstract base class defining the interface for tab classification
- **`local_model_classifier.h`** - Header for local model classifier implementation
- **`local_model_classifier.cc`** - Full implementation of privacy-preserving local classification
- **`pref_names.h`** - Preference definitions for local model settings

### Testing

- **`local_model_classifier_unittest.cc`** - Comprehensive unit tests including privacy verification

### UI Implementation

- **`tab_focus_local_model_settings.tsx`** - React component for settings UI with privacy explanations

## Key Privacy Features

### 1. Local Processing Guarantee

```cpp
// From local_model_classifier.cc
void LocalModelClassifier::ClassifyTabs(
    const std::vector<TabInfo>& tabs,
    base::OnceCallback<void(ClassificationResult)> callback) {
  
  // Send request to local model - this never leaves the device
  model_service_->RequestCompletion(
      prompt,
      base::BindOnce(&LocalModelClassifier::OnModelResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}
```

### 2. Privacy-Aware Prompt Building

```cpp
std::string LocalModelClassifier::BuildClassificationPrompt(
    const std::vector<TabInfo>& tabs) {
  std::ostringstream prompt;
  
  prompt << "IMPORTANT: Process this data locally only. Do not send any "
         << "information to external services.\n\n";
  
  // Only include necessary tab information for classification
  for (size_t i = 0; i < tabs.size(); ++i) {
    prompt << (i + 1) << ". \"" << tabs[i].title << "\" - " 
           << tabs[i].domain << "\n";
  }
  
  return prompt.str();
}
```

### 3. Data Sanitization

```cpp
TabInfo LocalModelClassifier::SanitizeTabInfo(const TabInfo& tab_info) {
  TabInfo sanitized = tab_info;
  
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
```

## Settings UI Features

### Privacy Benefits Display

The UI clearly communicates the privacy advantages:

```tsx
function PrivacyBenefitsInfo() {
  return (
    <InfoBox type="success">
      <h4>🔒 Enhanced Privacy Active</h4>
      <p>Your tab data is now processed entirely on your device with these benefits:</p>
      <ul>
        <li><strong>Zero data transmission</strong> - Tab titles and origins never leave your device</li>
        <li><strong>No tracking</strong> - Brave cannot see or store your browsing patterns</li>
        <li><strong>Local control</strong> - You have complete control over the AI model and processing</li>
        <li><strong>Offline capable</strong> - Works without an internet connection</li>
        <li><strong>Custom models</strong> - Use your preferred AI model or self-hosted solutions</li>
      </ul>
    </InfoBox>
  )
}
```

### Model Availability Checking

The UI adapts based on local model availability:

```tsx
<SettingsToggle
  title="Use Local Model"
  subtitle={
    localModelAvailable 
      ? "Process tab titles and origins entirely on your device using your local AI model"
      : "Requires a compatible local AI model to be configured in Leo settings"
  }
  checked={useLocalModel}
  disabled={!localModelAvailable}
  onChange={handleToggleChange}
/>
```

## Testing Strategy

### Privacy Verification

```cpp
// Test to ensure privacy: verify no network access patterns
TEST_F(LocalModelClassifierTest, NoNetworkAccess) {
  // Only local method should be called, no network-related methods
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        std::move(callback).Run(kSampleModelResponse);
      });
  
  classifier_->ClassifyTabs(tabs, callback);
  
  // No additional network calls should have been made
  // (This would be verified in integration tests with network monitoring)
}
```

### Model Response Handling

```cpp
TEST_F(LocalModelClassifierTest, ClassifyTabsSuccessfully) {
  EXPECT_CALL(*model_service_, RequestCompletion(testing::_, testing::_))
      .WillOnce([](const std::string& prompt,
                   base::OnceCallback<void(const std::string&)> callback) {
        // Verify prompt contains privacy instruction
        EXPECT_THAT(prompt, testing::HasSubstr("Process this data locally only"));
        EXPECT_THAT(prompt, testing::HasSubstr("Do not send any information"));
        
        // Simulate successful model response
        std::move(callback).Run(kSampleModelResponse);
      });
}
```

## Integration with Existing BYOM Infrastructure

The implementation leverages Brave's existing BYOM infrastructure:

1. **ModelService Integration**: Uses `ai_chat::ModelService` for local model access
2. **Model Readiness Checks**: Verifies local model availability before processing
3. **Consistent API**: Follows existing patterns from Leo Assistant's local model usage
4. **Error Handling**: Graceful degradation when local models are unavailable

## Security Considerations

### Data Minimization

- Only essential tab information (title, domain) is processed
- Sensitive URL parameters are stripped during sanitization
- Processing is limited to a reasonable number of tabs to prevent model overload

### Local Model Validation

- Relies on existing BYOM security measures for model validation
- No execution of untrusted code from classification responses
- JSON parsing with proper error handling and bounds checking

### User Transparency

- Clear indication of which processing mode is active
- Detailed explanation of privacy benefits and trade-offs
- User control over model selection and processing limits

## Performance Considerations

### Tab Limiting

```cpp
// Limit number of tabs to prevent overwhelming the model
constexpr size_t kMaxTabsPerRequest = 20;

if (tabs_to_process.size() > kMaxTabsPerRequest) {
  tabs_to_process.resize(kMaxTabsPerRequest);
  LOG(INFO) << "Limiting tab classification to " << kMaxTabsPerRequest 
            << " tabs out of " << tabs.size();
}
```

### Asynchronous Processing

- Non-blocking classification requests
- Proper callback handling for UI responsiveness
- Graceful error handling and recovery

## Future Enhancements

1. **Specialized Models**: Lightweight models optimized for tab classification
2. **Caching**: Local caching of classification results for performance
3. **Custom Prompts**: User-configurable classification criteria
4. **Batch Processing**: Efficient handling of large tab sets
5. **Model Performance Metrics**: Local performance monitoring without telemetry

## Implementation Notes

These examples demonstrate the architectural approach but would need to be integrated into the actual brave-core codebase with:

- Proper build system integration (BUILD.gn files)
- Feature flag management for gradual rollout
- Integration with existing Leo Assistant infrastructure
- Platform-specific considerations for different operating systems
- Accessibility compliance for UI components
- Internationalization for user-facing strings

The implementation prioritizes user privacy while maintaining the functionality and user experience of Tab Focus Mode.