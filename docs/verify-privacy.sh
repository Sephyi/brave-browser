#!/bin/bash

# Copyright (c) 2025 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.

# Tab Focus Mode Local Model Privacy Verification Script
# This script demonstrates how to verify that local-only processing is used
# when the local model option is enabled.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "🔒 Tab Focus Mode Local Model Privacy Verification"
echo "=================================================="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}➤${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Function to check if we're in a brave-core environment
check_environment() {
    print_step "Checking environment..."
    
    if [[ -f "${PROJECT_ROOT}/src/brave/package.json" ]]; then
        print_success "Brave-core development environment detected"
        return 0
    elif [[ -f "${PROJECT_ROOT}/README.md" ]] && grep -q "brave-core" "${PROJECT_ROOT}/README.md"; then
        print_warning "Brave-browser repository detected (build orchestration layer)"
        print_warning "This script demonstrates verification patterns for brave-core"
        return 1
    else
        print_error "Unknown environment - please run from brave-browser or brave-core repository"
        return 2
    fi
}

# Function to demonstrate privacy verification tests
demonstrate_privacy_tests() {
    print_step "Privacy Verification Test Patterns"
    echo ""
    
    cat << 'EOF'
The following test patterns verify that Tab Focus Mode local model processing
preserves privacy by ensuring no data leaves the device:

1. NETWORK ISOLATION TEST
   - Monitor all network connections during local classification
   - Verify no requests to *.brave.com domains
   - Confirm no external AI service endpoints are contacted

2. DATA FLOW VERIFICATION
   - Mock the ModelService to track method calls
   - Verify only local processing methods are invoked
   - Ensure no remote completion endpoints are used

3. PREFERENCE VALIDATION
   - Test that brave.tab_focus.use_local_model controls behavior
   - Verify settings UI accurately reflects processing mode
   - Confirm fallback behavior when local model unavailable

4. PROMPT SANITIZATION
   - Verify sensitive data is removed from prompts
   - Check that privacy instructions are included
   - Validate tab data minimization

Example test implementation:
EOF

    cat << 'EOF'

TEST_F(TabFocusPrivacyTest, LocalModePreservesPrivacy) {
  // Enable local mode
  pref_service_->SetBoolean(tab_focus::prefs::kUseLocalModel, true);
  
  // Set up network monitoring
  auto network_monitor = std::make_unique<NetworkRequestMonitor>();
  
  // Perform tab classification with sensitive data
  std::vector<TabInfo> sensitive_tabs = {
    {.title = "Personal Bank Account", .domain = "mybank.com", .tab_id = 1},
    {.title = "Private Email", .domain = "mail.example.com", .tab_id = 2}
  };
  
  bool classification_completed = false;
  service_->ClassifyTabs(sensitive_tabs, 
      base::BindOnce([&](ClassificationResult result) {
        EXPECT_TRUE(result.IsSuccess());
        classification_completed = true;
      }));
  
  EXPECT_TRUE(classification_completed);
  
  // CRITICAL: Verify no network requests were made
  EXPECT_EQ(0, network_monitor->GetRequestCount("*.brave.com"));
  EXPECT_EQ(0, network_monitor->GetRequestCount("*leo*"));
  EXPECT_EQ(0, network_monitor->GetTotalExternalRequests());
}

EOF
}

# Function to show integration test patterns
demonstrate_integration_tests() {
    print_step "Integration Test Patterns"
    echo ""
    
    cat << 'EOF'
Integration tests verify end-to-end privacy preservation:

1. BROWSER TEST WITH NETWORK MONITORING
   - Launch browser with local model enabled
   - Open tabs with sensitive information
   - Trigger Tab Focus Mode classification
   - Monitor network traffic to ensure no external requests

2. SETTINGS UI VALIDATION
   - Test privacy benefits are displayed correctly
   - Verify model availability checking works
   - Confirm graceful fallback when local model unavailable

3. BYOM INFRASTRUCTURE INTEGRATION
   - Verify compatibility with existing local models
   - Test with different model types (ONNX, GGML, etc.)
   - Validate error handling for model failures

Example browser test:
EOF

    cat << 'EOF'

class TabFocusLocalModelBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUp() override {
    // Enable local model feature
    feature_list_.InitAndEnableFeature(features::kBraveTabFocusLocalModel);
    InProcessBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    // Configure local model for testing
    auto* model_service = GetModelService();
    model_service->SetLocalModelForTesting(CreateTestModel());
    
    // Enable local processing in preferences
    browser()->profile()->GetPrefs()->SetBoolean(
        tab_focus::prefs::kUseLocalModel, true);
  }
};

IN_PROC_BROWSER_TEST_F(TabFocusLocalModelBrowserTest, 
                       ClassificationWithNoNetworkRequests) {
  // Set up network monitoring
  auto network_observer = std::make_unique<NetworkTrafficObserver>();
  
  // Open tabs with sensitive content
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("https://mybank.com/account")));
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), GURL("https://privatesite.com/personal")));
  
  // Trigger Tab Focus Mode
  auto* tab_focus_service = GetTabFocusService(browser()->profile());
  
  bool classification_done = false;
  tab_focus_service->ClassifyCurrentTabs(
      base::BindOnce([&](ClassificationResult result) {
        EXPECT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.groups.empty());
        classification_done = true;
      }));
  
  // Wait for classification to complete
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(classification_done);
  
  // CRITICAL: Verify no external network requests
  EXPECT_EQ(0, network_observer->GetExternalRequestCount());
  EXPECT_FALSE(network_observer->HasRequestsToPattern("*.brave.com"));
}

EOF
}

# Function to show manual testing steps
show_manual_testing() {
    print_step "Manual Privacy Verification Steps"
    echo ""
    
    cat << 'EOF'
Manual testing steps to verify privacy preservation:

SETUP:
1. Configure a local AI model in Leo Assistant settings
2. Enable "Use Local Model" in Tab Focus Mode settings
3. Set up network monitoring (e.g., Wireshark, browser dev tools)

TESTING:
1. Open multiple tabs with varying content:
   - Personal/sensitive sites (banking, email, documents)
   - Work-related tabs (company tools, project management)
   - Entertainment tabs (news, social media, videos)

2. Trigger Tab Focus Mode classification:
   - Open Tab Search (Ctrl+Shift+A or Cmd+Shift+A)
   - Navigate to "Tab Organization" tab
   - Click "Suggest Groups" or enter custom focus topic

3. Monitor network traffic during classification:
   - Verify NO requests to brave.com domains
   - Verify NO requests to AI service endpoints
   - Confirm only local processing occurs

4. Verify classification results:
   - Check that tabs are appropriately grouped
   - Confirm groups have meaningful names
   - Validate that classification quality is reasonable

5. Test edge cases:
   - Very long tab titles (>200 characters)
   - Tabs with special characters or non-English content
   - Large number of tabs (>20)
   - Local model temporarily unavailable

EXPECTED BEHAVIOR:
✓ No network requests during classification
✓ Processing completes using local model only
✓ Privacy benefits displayed in settings UI
✓ Graceful degradation when local model unavailable
✓ User clearly informed about processing mode

EOF
}

# Function to validate code examples
validate_examples() {
    print_step "Validating code examples..."
    
    local examples_dir="${PROJECT_ROOT}/docs/examples"
    local files=(
        "tab_classifier.h"
        "local_model_classifier.h" 
        "local_model_classifier.cc"
        "local_model_classifier_unittest.cc"
        "pref_names.h"
        "tab_focus_local_model_settings.tsx"
    )
    
    for file in "${files[@]}"; do
        if [[ -f "${examples_dir}/${file}" ]]; then
            print_success "Found example: ${file}"
        else
            print_error "Missing example: ${file}"
        fi
    done
    
    echo ""
    print_step "Example files demonstrate:"
    echo "  - Privacy-preserving local classification implementation"
    echo "  - Comprehensive unit testing with privacy verification"
    echo "  - Settings UI with clear privacy benefit communication"
    echo "  - Integration with existing BYOM infrastructure"
    echo "  - Proper error handling and graceful degradation"
}

# Function to show security checklist
show_security_checklist() {
    print_step "Security and Privacy Checklist"
    echo ""
    
    cat << 'EOF'
Pre-deployment security verification checklist:

□ DATA FLOW ANALYSIS
  □ Verify no tab data transmitted in local mode
  □ Confirm URL sanitization removes sensitive parameters  
  □ Validate prompt construction includes privacy instructions
  □ Check that model responses don't leak processed data

□ NETWORK ISOLATION
  □ Unit tests verify no network calls in local mode
  □ Integration tests monitor actual network traffic
  □ Browser tests confirm end-to-end privacy preservation
  □ Manual verification with network monitoring tools

□ USER INTERFACE
  □ Settings clearly explain privacy benefits and trade-offs
  □ Model availability status accurately reflected
  □ Fallback behavior properly communicated
  □ Error messages don't expose internal details

□ BYOM INTEGRATION
  □ Local model validation uses existing security measures
  □ Model loading follows established security patterns
  □ No execution of untrusted code from model responses
  □ Proper error handling for model failures

□ PREFERENCE MANAGEMENT
  □ Settings properly control classification behavior
  □ Default values prioritize user privacy
  □ Preference changes take effect immediately
  □ Migration from existing settings handled correctly

□ TESTING COVERAGE
  □ Unit tests cover all code paths
  □ Privacy-specific tests verify no data leakage
  □ Integration tests validate end-to-end behavior
  □ Performance tests ensure reasonable resource usage

□ DOCUMENTATION
  □ Privacy benefits clearly explained to users
  □ Developer documentation covers security considerations
  □ Code comments explain privacy-critical sections
  □ User-facing help content addresses common questions

EOF
}

# Main execution
main() {
    echo "This script demonstrates privacy verification patterns for"
    echo "Tab Focus Mode local model implementation."
    echo ""
    
    local env_status
    check_environment
    env_status=$?
    
    echo ""
    
    if [[ $env_status -eq 0 ]]; then
        print_success "Ready for implementation in brave-core environment"
    elif [[ $env_status -eq 1 ]]; then
        print_warning "Code examples available for brave-core implementation"
    else
        print_error "Please run from a Brave repository"
        exit 1
    fi
    
    echo ""
    validate_examples
    echo ""
    demonstrate_privacy_tests
    echo ""
    demonstrate_integration_tests
    echo ""
    show_manual_testing
    echo ""
    show_security_checklist
    echo ""
    
    print_success "Privacy verification patterns documented"
    echo ""
    echo "Next steps:"
    echo "1. Implement the code patterns in brave-core"
    echo "2. Add comprehensive privacy verification tests"
    echo "3. Integrate with existing BYOM infrastructure"
    echo "4. Update settings UI with privacy benefits"
    echo "5. Perform security review before deployment"
    echo ""
    print_step "For implementation details, see: docs/tab-focus-local-model.md"
}

# Execute main function
main "$@"