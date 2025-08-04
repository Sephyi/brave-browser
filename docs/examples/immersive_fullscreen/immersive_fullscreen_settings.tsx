// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import React, { useState, useEffect } from 'react'
import { 
  SettingsRow, 
  SettingsToggle, 
  SettingsSection,
  SettingsSlider,
  InfoBox,
  Button
} from '../../../ui/webui/settings/components'

interface ImmersiveFullscreenSettingsProps {
  onSettingChanged: (pref: string, value: any) => void
  immersiveEnabled: boolean
  autoHideDelayMs: number
  topEdgeSensitivityPx: number
  animationDurationMs: number
  showOnAddressBarFocus: boolean
  showOnKeyboardActivity: boolean
  hasSeenIntroduction: boolean
  respectLegacyFullscreenPref: boolean
  legacyFullscreenToolbarEnabled: boolean
}

/**
 * Settings component for Immersive Fullscreen Mode.
 * Provides user controls for customizing the immersive browsing experience
 * with auto-hiding UI elements in fullscreen mode.
 */
export function ImmersiveFullscreenSettings({
  onSettingChanged,
  immersiveEnabled,
  autoHideDelayMs,
  topEdgeSensitivityPx,
  animationDurationMs,
  showOnAddressBarFocus,
  showOnKeyboardActivity,
  hasSeenIntroduction,
  respectLegacyFullscreenPref,
  legacyFullscreenToolbarEnabled
}: ImmersiveFullscreenSettingsProps) {
  const [showAdvanced, setShowAdvanced] = useState(false)
  const [showPreview, setShowPreview] = useState(false)

  // Check if immersive mode is effectively disabled due to legacy preference
  const isDisabledByLegacyPref = respectLegacyFullscreenPref && legacyFullscreenToolbarEnabled

  const handleToggleImmersive = (enabled: boolean) => {
    onSettingChanged('brave.immersive_fullscreen.enabled', enabled)
    
    // Show introduction if enabling for the first time
    if (enabled && !hasSeenIntroduction) {
      // Introduction will be shown automatically by the controller
    }
  }

  const handleAutoHideDelayChange = (value: number) => {
    onSettingChanged('brave.immersive_fullscreen.auto_hide_delay_ms', value)
  }

  const handleTopEdgeSensitivityChange = (value: number) => {
    onSettingChanged('brave.immersive_fullscreen.top_edge_sensitivity_px', value)
  }

  const handleAnimationDurationChange = (value: number) => {
    onSettingChanged('brave.immersive_fullscreen.animation_duration_ms', value)
  }

  const handleAddressBarFocusToggle = (enabled: boolean) => {
    onSettingChanged('brave.immersive_fullscreen.show_on_address_bar_focus', enabled)
  }

  const handleKeyboardActivityToggle = (enabled: boolean) => {
    onSettingChanged('brave.immersive_fullscreen.show_on_keyboard_activity', enabled)
  }

  const handleLegacyPrefToggle = (enabled: boolean) => {
    onSettingChanged('brave.immersive_fullscreen.respect_legacy_fullscreen_pref', enabled)
  }

  const resetToDefaults = () => {
    onSettingChanged('brave.immersive_fullscreen.auto_hide_delay_ms', 2000)
    onSettingChanged('brave.immersive_fullscreen.top_edge_sensitivity_px', 5)
    onSettingChanged('brave.immersive_fullscreen.animation_duration_ms', 300)
    onSettingChanged('brave.immersive_fullscreen.show_on_address_bar_focus', true)
    onSettingChanged('brave.immersive_fullscreen.show_on_keyboard_activity', false)
  }

  return (
    <SettingsSection title="Immersive Fullscreen Mode">
      <SettingsRow>
        <div className="setting-description">
          <h3>Distraction-Free Browsing</h3>
          <p>
            Automatically hide toolbars, tabs, and bookmarks bar in fullscreen mode 
            for a clean, immersive browsing experience. UI elements reappear when you 
            move your mouse to the top of the screen.
          </p>
        </div>
      </SettingsRow>

      {/* Legacy preference conflict warning */}
      {isDisabledByLegacyPref && (
        <InfoBox type="warning">
          <h4>⚠️ Immersive Mode Disabled</h4>
          <p>
            Immersive fullscreen mode is currently disabled because "Always Show Toolbar in Full Screen" 
            is enabled. To use immersive mode, disable the legacy preference or turn off 
            "Respect Legacy Fullscreen Preference" below.
          </p>
        </InfoBox>
      )}

      <SettingsRow>
        <SettingsToggle
          title="Enable Immersive Fullscreen Mode"
          subtitle={
            isDisabledByLegacyPref
              ? "Disabled due to legacy fullscreen preference - see warning above"
              : "Automatically hide UI elements in fullscreen mode for distraction-free browsing"
          }
          checked={immersiveEnabled}
          disabled={isDisabledByLegacyPref}
          onChange={handleToggleImmersive}
        />
      </SettingsRow>

      {immersiveEnabled && !isDisabledByLegacyPref && (
        <>
          <ImmersiveFeatureDescription />
          
          {/* Quick settings */}
          <SettingsRow>
            <SettingsToggle
              title="Show UI on Address Bar Focus"
              subtitle="Always reveal the toolbar when typing in the address bar"
              checked={showOnAddressBarFocus}
              onChange={handleAddressBarFocusToggle}
            />
          </SettingsRow>

          <SettingsRow>
            <SettingsSlider
              title="Auto-hide Delay"
              subtitle={`UI disappears after ${autoHideDelayMs / 1000} seconds of inactivity`}
              min={500}
              max={10000}
              step={500}
              value={autoHideDelayMs}
              onChange={handleAutoHideDelayChange}
              formatValue={(value) => `${value / 1000}s`}
            />
          </SettingsRow>

          <SettingsRow>
            <Button
              variant="secondary"
              onClick={() => setShowPreview(!showPreview)}
            >
              {showPreview ? 'Hide Preview' : 'Preview Immersive Mode'}
            </Button>
            
            <Button
              variant="secondary"
              onClick={() => setShowAdvanced(!showAdvanced)}
            >
              {showAdvanced ? 'Hide Advanced Settings' : 'Advanced Settings'}
            </Button>
          </SettingsRow>

          {showPreview && (
            <ImmersivePreviewDemo 
              autoHideDelayMs={autoHideDelayMs}
              animationDurationMs={animationDurationMs}
            />
          )}

          {showAdvanced && (
            <ImmersiveAdvancedSettings
              topEdgeSensitivityPx={topEdgeSensitivityPx}
              animationDurationMs={animationDurationMs}
              showOnKeyboardActivity={showOnKeyboardActivity}
              respectLegacyFullscreenPref={respectLegacyFullscreenPref}
              onTopEdgeSensitivityChange={handleTopEdgeSensitivityChange}
              onAnimationDurationChange={handleAnimationDurationChange}
              onKeyboardActivityToggle={handleKeyboardActivityToggle}
              onLegacyPrefToggle={handleLegacyPrefToggle}
              onResetToDefaults={resetToDefaults}
            />
          )}
        </>
      )}
    </SettingsSection>
  )
}

/**
 * Component describing immersive mode features and benefits
 */
function ImmersiveFeatureDescription() {
  return (
    <InfoBox type="info">
      <h4>🎯 Immersive Browsing Features</h4>
      <p>When immersive fullscreen mode is active:</p>
      <ul>
        <li><strong>Auto-hide UI</strong> - Toolbars and tabs automatically disappear for clean viewing</li>
        <li><strong>Smart reveal</strong> - Move your mouse to the top edge to show UI elements</li>
        <li><strong>Focus aware</strong> - UI stays visible when using the address bar or keyboard shortcuts</li>
        <li><strong>Smooth animations</strong> - Gentle transitions that don't disrupt your workflow</li>
        <li><strong>Safari-style UX</strong> - Familiar behavior for macOS users</li>
      </ul>
      <p className="note">
        <em>Tip: Press F11 to enter/exit fullscreen mode and experience immersive browsing</em>
      </p>
    </InfoBox>
  )
}

/**
 * Advanced settings for power users
 */
function ImmersiveAdvancedSettings({
  topEdgeSensitivityPx,
  animationDurationMs,
  showOnKeyboardActivity,
  respectLegacyFullscreenPref,
  onTopEdgeSensitivityChange,
  onAnimationDurationChange,
  onKeyboardActivityToggle,
  onLegacyPrefToggle,
  onResetToDefaults
}: {
  topEdgeSensitivityPx: number
  animationDurationMs: number
  showOnKeyboardActivity: boolean
  respectLegacyFullscreenPref: boolean
  onTopEdgeSensitivityChange: (value: number) => void
  onAnimationDurationChange: (value: number) => void
  onKeyboardActivityToggle: (enabled: boolean) => void
  onLegacyPrefToggle: (enabled: boolean) => void
  onResetToDefaults: () => void
}) {
  return (
    <div className="advanced-settings">
      <h4>Advanced Configuration</h4>
      
      <SettingsRow>
        <SettingsSlider
          title="Top Edge Sensitivity"
          subtitle={`Mouse detection area: ${topEdgeSensitivityPx} pixels from top`}
          min={1}
          max={20}
          step={1}
          value={topEdgeSensitivityPx}
          onChange={onTopEdgeSensitivityChange}
          formatValue={(value) => `${value}px`}
        />
      </SettingsRow>

      <SettingsRow>
        <SettingsSlider
          title="Animation Speed"
          subtitle={`UI transitions take ${animationDurationMs} milliseconds`}
          min={100}
          max={1000}
          step={50}
          value={animationDurationMs}
          onChange={onAnimationDurationChange}
          formatValue={(value) => `${value}ms`}
        />
      </SettingsRow>

      <SettingsRow>
        <SettingsToggle
          title="Show UI on Any Keyboard Activity"
          subtitle="Reveal UI on any keypress (not just address bar focus)"
          checked={showOnKeyboardActivity}
          onChange={onKeyboardActivityToggle}
        />
      </SettingsRow>

      <SettingsRow>
        <SettingsToggle
          title="Respect Legacy Fullscreen Preference"
          subtitle="Disable immersive mode when 'Always Show Toolbar in Full Screen' is enabled"
          checked={respectLegacyFullscreenPref}
          onChange={onLegacyPrefToggle}
        />
      </SettingsRow>

      <SettingsRow>
        <Button
          variant="secondary"
          onClick={onResetToDefaults}
        >
          Reset to Defaults
        </Button>
      </SettingsRow>
    </div>
  )
}

/**
 * Interactive preview of immersive mode behavior
 */
function ImmersivePreviewDemo({ 
  autoHideDelayMs, 
  animationDurationMs 
}: { 
  autoHideDelayMs: number
  animationDurationMs: number
}) {
  const [isUIVisible, setIsUIVisible] = useState(true)
  const [isHovering, setIsHovering] = useState(false)
  const [autoHideTimer, setAutoHideTimer] = useState<NodeJS.Timeout | null>(null)

  const handleMouseEnter = () => {
    setIsHovering(true)
    setIsUIVisible(true)
    if (autoHideTimer) {
      clearTimeout(autoHideTimer)
      setAutoHideTimer(null)
    }
  }

  const handleMouseLeave = () => {
    setIsHovering(false)
    const timer = setTimeout(() => {
      setIsUIVisible(false)
    }, autoHideDelayMs)
    setAutoHideTimer(timer)
  }

  useEffect(() => {
    // Cleanup timer on unmount
    return () => {
      if (autoHideTimer) {
        clearTimeout(autoHideTimer)
      }
    }
  }, [autoHideTimer])

  return (
    <div className="immersive-preview">
      <InfoBox type="demo">
        <h4>🖥️ Interactive Preview</h4>
        <p>Move your mouse over the preview area to see how immersive mode works:</p>
        
        <div 
          className="preview-container"
          onMouseEnter={handleMouseEnter}
          onMouseLeave={handleMouseLeave}
          style={{
            position: 'relative',
            height: '200px',
            backgroundColor: '#1a1a1a',
            borderRadius: '8px',
            overflow: 'hidden',
            cursor: 'pointer'
          }}
        >
          {/* Simulated browser UI */}
          <div 
            className="preview-ui"
            style={{
              position: 'absolute',
              top: 0,
              left: 0,
              right: 0,
              height: '40px',
              backgroundColor: '#333',
              borderBottom: '1px solid #555',
              display: 'flex',
              alignItems: 'center',
              padding: '0 12px',
              transform: isUIVisible ? 'translateY(0)' : 'translateY(-100%)',
              opacity: isUIVisible ? 1 : 0,
              transition: `all ${animationDurationMs}ms cubic-bezier(0.4, 0.0, 0.2, 1)`,
              color: '#fff',
              fontSize: '12px'
            }}
          >
            <div>🌐 brave://example.com</div>
          </div>
          
          {/* Simulated content area */}
          <div 
            style={{
              position: 'absolute',
              top: isUIVisible ? '40px' : '0',
              left: 0,
              right: 0,
              bottom: 0,
              backgroundColor: '#2a2a2a',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              color: '#ccc',
              transition: `top ${animationDurationMs}ms cubic-bezier(0.4, 0.0, 0.2, 1)`,
              fontSize: '14px'
            }}
          >
            <div style={{ textAlign: 'center' }}>
              <div>📄 Webpage Content</div>
              <div style={{ fontSize: '12px', marginTop: '8px', opacity: 0.7 }}>
                {isHovering ? 'UI Visible' : isUIVisible ? 'Waiting to hide...' : 'UI Hidden'}
              </div>
            </div>
          </div>
          
          {/* Top edge indicator */}
          <div 
            style={{
              position: 'absolute',
              top: 0,
              left: 0,
              right: 0,
              height: '5px',
              background: 'linear-gradient(to bottom, rgba(255,255,255,0.2) 0%, transparent 100%)',
              opacity: isHovering ? 1 : 0,
              transition: 'opacity 150ms ease-in-out'
            }}
          />
        </div>
        
        <p style={{ fontSize: '12px', marginTop: '8px', color: '#666' }}>
          Auto-hide delay: {autoHideDelayMs / 1000}s | Animation: {animationDurationMs}ms
        </p>
      </InfoBox>
    </div>
  )
}

// CSS styles for the settings components
export const immersiveFullscreenSettingsStyles = `
.immersive-preview .preview-container {
  user-select: none;
  border: 2px solid var(--leo-color-divider-subtle);
}

.immersive-preview .preview-container:hover {
  border-color: var(--leo-color-icon-interactive);
}

.advanced-settings {
  margin-top: 16px;
  padding: 16px;
  background-color: var(--leo-color-container-highlight);
  border-radius: 8px;
  border: 1px solid var(--leo-color-divider-subtle);
}

.advanced-settings h4 {
  margin: 0 0 16px 0;
  color: var(--leo-color-text-primary);
  font-size: 1.1em;
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

.note {
  margin-top: 12px;
  font-size: 0.9em;
  color: var(--leo-color-text-tertiary);
  font-style: italic;
}

.demo-info {
  background-color: var(--leo-color-container-highlight);
  border: 1px solid var(--leo-color-divider-subtle);
  border-radius: 8px;
  padding: 16px;
  margin: 16px 0;
}

.demo-info h4 {
  margin: 0 0 8px 0;
  color: var(--leo-color-text-primary);
}

.demo-info ul {
  margin: 8px 0;
  padding-left: 20px;
}

.demo-info li {
  margin-bottom: 4px;
  color: var(--leo-color-text-secondary);
}
`