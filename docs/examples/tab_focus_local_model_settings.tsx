// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import React, { useState, useEffect } from 'react'
import { 
  SettingsRow, 
  SettingsToggle, 
  SettingsSection,
  InfoBox 
} from '../../../ui/webui/settings/components'

interface TabFocusLocalModelProps {
  onSettingChanged: (pref: string, value: boolean) => void
  useLocalModel: boolean
  localModelAvailable: boolean
  isLeoEnabled: boolean
}

/**
 * Settings component for Tab Focus Mode local model option.
 * This component allows users to enable privacy-preserving local classification
 * instead of sending tab data to Brave servers.
 */
export function TabFocusLocalModelSettings({
  onSettingChanged,
  useLocalModel,
  localModelAvailable,
  isLeoEnabled
}: TabFocusLocalModelProps) {
  const [showPrivacyInfo, setShowPrivacyInfo] = useState(false)

  // Show privacy benefits when local model is enabled
  useEffect(() => {
    setShowPrivacyInfo(useLocalModel)
  }, [useLocalModel])

  const handleToggleChange = (enabled: boolean) => {
    onSettingChanged('brave.tab_focus.use_local_model', enabled)
    
    // Show notification about privacy benefits on first enable
    if (enabled) {
      setShowPrivacyInfo(true)
    }
  }

  // Don't show if Leo is not enabled
  if (!isLeoEnabled) {
    return null
  }

  return (
    <SettingsSection title="Tab Focus Mode">
      <SettingsRow>
        <div className="setting-description">
          <h3>Privacy-Preserving Classification</h3>
          <p>
            Choose how Tab Focus Mode processes your tab information for AI-powered organization.
          </p>
        </div>
      </SettingsRow>

      <SettingsRow>
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
      </SettingsRow>

      {!localModelAvailable && (
        <InfoBox type="warning">
          <h4>Local Model Not Available</h4>
          <p>
            To use local processing, you need to configure a compatible AI model in 
            <a href="/leo-assistant/customization">Leo Assistant settings</a>.
            This includes BYOM (Bring Your Own Model) configurations or system-installed models.
          </p>
        </InfoBox>
      )}

      {showPrivacyInfo && useLocalModel && (
        <PrivacyBenefitsInfo />
      )}

      {!useLocalModel && (
        <RemoteProcessingInfo />
      )}
    </SettingsSection>
  )
}

/**
 * Component showing privacy benefits of local processing
 */
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
      <p className="note">
        <em>Note: Classification accuracy may vary depending on your local model capabilities.</em>
      </p>
    </InfoBox>
  )
}

/**
 * Component explaining remote processing (default mode)
 */
function RemoteProcessingInfo() {
  return (
    <InfoBox type="info">
      <h4>ℹ️ Remote Processing Active</h4>
      <p>
        Tab Focus Mode is currently using Brave's cloud-based AI for classification. 
        Tab titles and domains are sent to Brave servers for processing.
      </p>
      <p>
        <strong>Privacy considerations:</strong>
      </p>
      <ul>
        <li>Tab titles and domain names are transmitted to Brave servers</li>
        <li>Data is processed according to Brave's privacy policy</li>
        <li>Classification results are returned to organize your tabs</li>
        <li>Generally provides more accurate results with specialized models</li>
      </ul>
      <p>
        Enable "Use Local Model" above for maximum privacy if you have a compatible local AI model configured.
      </p>
    </InfoBox>
  )
}

/**
 * Settings component for Tab Focus Mode advanced options
 */
export function TabFocusAdvancedSettings({
  onSettingChanged,
  maxTabsForProcessing,
  showLocalModeNotification
}: {
  onSettingChanged: (pref: string, value: any) => void
  maxTabsForProcessing: number
  showLocalModeNotification: boolean
}) {
  const [expanded, setExpanded] = useState(false)

  return (
    <SettingsSection title="Advanced Options" collapsible>
      <button 
        className="section-toggle"
        onClick={() => setExpanded(!expanded)}
      >
        {expanded ? 'Hide' : 'Show'} Advanced Options
      </button>

      {expanded && (
        <>
          <SettingsRow>
            <label htmlFor="max-tabs-input">
              Maximum Tabs for Local Processing
            </label>
            <input
              id="max-tabs-input"
              type="number"
              min="5"
              max="50"
              value={maxTabsForProcessing}
              onChange={(e) => 
                onSettingChanged(
                  'brave.tab_focus.max_tabs_local_processing',
                  parseInt(e.target.value)
                )
              }
            />
            <p className="help-text">
              Limits the number of tabs processed in local mode to prevent overwhelming your AI model.
            </p>
          </SettingsRow>

          <SettingsRow>
            <SettingsToggle
              title="Show Local Mode Notifications"
              subtitle="Display privacy notifications when switching to local processing mode"
              checked={showLocalModeNotification}
              onChange={(enabled) =>
                onSettingChanged('brave.tab_focus.show_local_mode_notification', enabled)
              }
            />
          </SettingsRow>
        </>
      )}
    </SettingsSection>
  )
}

// CSS styles for the components
export const tabFocusLocalModelStyles = `
.privacy-benefits {
  margin-top: 16px;
  padding: 16px;
  background-color: var(--leo-color-container-highlight);
  border: 1px solid var(--leo-color-divider-subtle);
  border-radius: 8px;
}

.privacy-benefits h4 {
  margin: 0 0 8px 0;
  color: var(--leo-color-text-primary);
}

.privacy-benefits ul {
  margin: 8px 0;
  padding-left: 20px;
}

.privacy-benefits li {
  margin-bottom: 4px;
  color: var(--leo-color-text-secondary);
}

.privacy-benefits .note {
  margin-top: 12px;
  font-size: 0.9em;
  color: var(--leo-color-text-tertiary);
}

.setting-description h3 {
  margin: 0 0 4px 0;
  font-size: 1.1em;
  font-weight: 600;
}

.setting-description p {
  margin: 0;
  color: var(--leo-color-text-secondary);
}

.section-toggle {
  background: none;
  border: none;
  color: var(--leo-color-text-interactive);
  cursor: pointer;
  font-size: 0.9em;
  text-decoration: underline;
}

.help-text {
  font-size: 0.85em;
  color: var(--leo-color-text-tertiary);
  margin-top: 4px;
}

#max-tabs-input {
  width: 80px;
  padding: 4px 8px;
  border: 1px solid var(--leo-color-divider-subtle);
  border-radius: 4px;
  background-color: var(--leo-color-container-background);
  color: var(--leo-color-text-primary);
}
`