// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * @fileoverview Event handlers for Immersive Fullscreen Mode
 * 
 * This file contains JavaScript event handlers that would integrate with
 * the browser's event system to provide immersive fullscreen functionality.
 * These handlers work with the C++ controller to manage UI visibility
 * and user interactions.
 */

/**
 * Event handler manager for immersive fullscreen mode
 */
class ImmersiveFullscreenEventHandler {
  constructor() {
    this.isEnabled = false
    this.isUIVisible = true
    this.mouseTrackingActive = false
    this.lastMousePosition = { x: 0, y: 0 }
    this.topEdgeSensitivity = 5 // pixels
    this.autoHideTimer = null
    this.autoHideDelay = 2000 // milliseconds
    
    // Bind event handlers
    this.handleMouseMove = this.handleMouseMove.bind(this)
    this.handleKeyDown = this.handleKeyDown.bind(this)
    this.handleAddressBarFocus = this.handleAddressBarFocus.bind(this)
    this.handleAddressBarBlur = this.handleAddressBarBlur.bind(this)
    this.handleFullscreenChange = this.handleFullscreenChange.bind(this)
    this.handleWindowResize = this.handleWindowResize.bind(this)
    
    this.initializeEventListeners()
  }

  /**
   * Initialize all event listeners for immersive mode
   */
  initializeEventListeners() {
    // Mouse movement tracking
    document.addEventListener('mousemove', this.handleMouseMove, { passive: true })
    
    // Keyboard events
    document.addEventListener('keydown', this.handleKeyDown, { passive: true })
    
    // Fullscreen change events
    document.addEventListener('fullscreenchange', this.handleFullscreenChange)
    document.addEventListener('webkitfullscreenchange', this.handleFullscreenChange)
    document.addEventListener('mozfullscreenchange', this.handleFullscreenChange)
    document.addEventListener('MSFullscreenChange', this.handleFullscreenChange)
    
    // Window resize events
    window.addEventListener('resize', this.handleWindowResize, { passive: true })
    
    // Address bar events (if available)
    const addressBar = document.querySelector('#omnibox-input, .location-bar-input')
    if (addressBar) {
      addressBar.addEventListener('focus', this.handleAddressBarFocus)
      addressBar.addEventListener('blur', this.handleAddressBarBlur)
    }
    
    // Tab change events
    this.observeTabChanges()
  }

  /**
   * Handle mouse movement for top-edge detection
   * @param {MouseEvent} event - Mouse move event
   */
  handleMouseMove(event) {
    if (!this.isEnabled) return

    this.lastMousePosition = { x: event.clientX, y: event.clientY }
    
    const isInTopEdge = this.isMouseInTopEdge(event.clientY)
    
    if (isInTopEdge && !this.isUIVisible) {
      this.revealUI('mouse_hover')
    } else if (!isInTopEdge && this.isUIVisible && !this.isAddressBarFocused()) {
      this.scheduleAutoHide()
    }
    
    // Notify C++ controller about mouse position
    if (window.chrome && window.chrome.immersiveFullscreen) {
      window.chrome.immersiveFullscreen.onMouseMoved(event.screenX, event.screenY)
    }
  }

  /**
   * Handle keyboard events that might trigger UI reveal
   * @param {KeyboardEvent} event - Keyboard event
   */
  handleKeyDown(event) {
    if (!this.isEnabled) return

    // Always reveal UI for certain key combinations
    const shouldReveal = 
      event.ctrlKey || 
      event.metaKey || 
      event.altKey ||
      event.key === 'F11' ||
      event.key === 'Escape' ||
      event.key === 'Tab'

    if (shouldReveal) {
      this.revealUI('keyboard_shortcut')
    }
    
    // Notify C++ controller about keyboard activity
    if (window.chrome && window.chrome.immersiveFullscreen) {
      window.chrome.immersiveFullscreen.onKeyboardActivity(event.key, {
        ctrl: event.ctrlKey,
        alt: event.altKey,
        meta: event.metaKey,
        shift: event.shiftKey
      })
    }
  }

  /**
   * Handle address bar focus events
   * @param {FocusEvent} event - Focus event
   */
  handleAddressBarFocus(event) {
    if (!this.isEnabled) return
    
    this.revealUI('address_bar_focus')
    
    // Keep UI visible while address bar is focused
    this.cancelAutoHide()
    
    // Notify C++ controller
    if (window.chrome && window.chrome.immersiveFullscreen) {
      window.chrome.immersiveFullscreen.onAddressBarFocused()
    }
  }

  /**
   * Handle address bar blur events
   * @param {FocusEvent} event - Blur event
   */
  handleAddressBarBlur(event) {
    if (!this.isEnabled) return
    
    // Schedule auto-hide after address bar loses focus
    if (!this.isMouseInTopEdge(this.lastMousePosition.y)) {
      this.scheduleAutoHide()
    }
    
    // Notify C++ controller
    if (window.chrome && window.chrome.immersiveFullscreen) {
      window.chrome.immersiveFullscreen.onAddressBarBlurred()
    }
  }

  /**
   * Handle fullscreen change events
   * @param {Event} event - Fullscreen change event
   */
  handleFullscreenChange(event) {
    const isFullscreen = 
      document.fullscreenElement ||
      document.webkitFullscreenElement ||
      document.mozFullScreenElement ||
      document.msFullscreenElement

    if (isFullscreen) {
      this.enableImmersiveMode()
    } else {
      this.disableImmersiveMode()
    }
    
    // Notify C++ controller
    if (window.chrome && window.chrome.immersiveFullscreen) {
      window.chrome.immersiveFullscreen.onFullscreenStateChanged(!!isFullscreen)
    }
  }

  /**
   * Handle window resize events
   * @param {Event} event - Resize event
   */
  handleWindowResize(event) {
    if (!this.isEnabled) return
    
    // Update top edge sensitivity based on new window size
    this.updateTopEdgeBounds()
    
    // Briefly show UI when window is resized
    this.revealUI('window_resize')
    this.scheduleAutoHide()
  }

  /**
   * Check if mouse is in the top edge sensitive area
   * @param {number} mouseY - Mouse Y coordinate
   * @returns {boolean} True if mouse is in top edge
   */
  isMouseInTopEdge(mouseY) {
    return mouseY <= this.topEdgeSensitivity
  }

  /**
   * Check if address bar is currently focused
   * @returns {boolean} True if address bar has focus
   */
  isAddressBarFocused() {
    const addressBar = document.querySelector('#omnibox-input, .location-bar-input')
    return addressBar && addressBar === document.activeElement
  }

  /**
   * Reveal the UI with specified reason
   * @param {string} reason - Reason for revealing UI
   */
  revealUI(reason = 'unknown') {
    if (!this.isEnabled || this.isUIVisible) return

    this.isUIVisible = true
    this.cancelAutoHide()
    
    // Apply CSS classes for smooth animation
    document.body.classList.add('immersive-ui-revealed')
    document.body.classList.remove('immersive-ui-hidden')
    
    // Add specific reveal reason class
    document.body.classList.add(`immersive-reveal-${reason}`)
    
    // Log reveal for debugging (removed in production)
    console.debug(`Immersive UI revealed: ${reason}`)
    
    // Notify other components
    this.dispatchUIVisibilityEvent(true, reason)
  }

  /**
   * Hide the UI with smooth animation
   * @param {string} reason - Reason for hiding UI
   */
  hideUI(reason = 'auto_hide') {
    if (!this.isEnabled || !this.isUIVisible) return

    // Don't hide if address bar is focused
    if (this.isAddressBarFocused()) return

    this.isUIVisible = false
    
    // Apply CSS classes for smooth animation
    document.body.classList.add('immersive-ui-hidden')
    document.body.classList.remove('immersive-ui-revealed')
    
    // Remove reveal reason classes
    document.body.classList.forEach(className => {
      if (className.startsWith('immersive-reveal-')) {
        document.body.classList.remove(className)
      }
    })
    
    // Log hide for debugging (removed in production)
    console.debug(`Immersive UI hidden: ${reason}`)
    
    // Notify other components
    this.dispatchUIVisibilityEvent(false, reason)
  }

  /**
   * Schedule auto-hide timer
   */
  scheduleAutoHide() {
    this.cancelAutoHide()
    
    this.autoHideTimer = setTimeout(() => {
      this.hideUI('auto_hide_timer')
    }, this.autoHideDelay)
  }

  /**
   * Cancel pending auto-hide timer
   */
  cancelAutoHide() {
    if (this.autoHideTimer) {
      clearTimeout(this.autoHideTimer)
      this.autoHideTimer = null
    }
  }

  /**
   * Enable immersive fullscreen mode
   */
  enableImmersiveMode() {
    if (this.isEnabled) return

    this.isEnabled = true
    this.isUIVisible = false
    
    // Apply immersive mode CSS
    document.body.classList.add('immersive-fullscreen-enabled')
    document.body.classList.add('immersive-ui-hidden')
    
    // Start with UI hidden unless there's a reason to show it
    if (this.isAddressBarFocused() || this.isMouseInTopEdge(this.lastMousePosition.y)) {
      this.revealUI('initial_state')
    }
    
    console.debug('Immersive fullscreen mode enabled')
    
    // Dispatch custom event
    this.dispatchModeToggleEvent(true)
  }

  /**
   * Disable immersive fullscreen mode
   */
  disableImmersiveMode() {
    if (!this.isEnabled) return

    this.isEnabled = false
    this.isUIVisible = true
    
    // Remove immersive mode CSS
    document.body.classList.remove('immersive-fullscreen-enabled')
    document.body.classList.remove('immersive-ui-hidden')
    document.body.classList.remove('immersive-ui-revealed')
    
    // Cancel any pending timers
    this.cancelAutoHide()
    
    console.debug('Immersive fullscreen mode disabled')
    
    // Dispatch custom event
    this.dispatchModeToggleEvent(false)
  }

  /**
   * Update top edge bounds based on current window size
   */
  updateTopEdgeBounds() {
    // Could be enhanced to handle multi-monitor setups
    // or different sensitivity areas
  }

  /**
   * Observe tab changes to briefly reveal UI
   */
  observeTabChanges() {
    // Listen for tab change events (implementation depends on browser internals)
    if (window.chrome && window.chrome.tabs) {
      window.chrome.tabs.onActivated.addListener(() => {
        if (this.isEnabled) {
          this.revealUI('tab_change')
          this.scheduleAutoHide()
        }
      })
    }
  }

  /**
   * Dispatch custom event for UI visibility changes
   * @param {boolean} visible - Whether UI is visible
   * @param {string} reason - Reason for change
   */
  dispatchUIVisibilityEvent(visible, reason) {
    const event = new CustomEvent('immersive-ui-visibility-changed', {
      detail: { visible, reason, timestamp: Date.now() }
    })
    document.dispatchEvent(event)
  }

  /**
   * Dispatch custom event for mode toggle
   * @param {boolean} enabled - Whether immersive mode is enabled
   */
  dispatchModeToggleEvent(enabled) {
    const event = new CustomEvent('immersive-mode-toggled', {
      detail: { enabled, timestamp: Date.now() }
    })
    document.dispatchEvent(event)
  }

  /**
   * Update configuration from preferences
   * @param {Object} config - Configuration object
   */
  updateConfig(config) {
    if (config.topEdgeSensitivity !== undefined) {
      this.topEdgeSensitivity = config.topEdgeSensitivity
    }
    if (config.autoHideDelay !== undefined) {
      this.autoHideDelay = config.autoHideDelay
    }
    
    console.debug('Immersive fullscreen config updated:', config)
  }

  /**
   * Cleanup event listeners
   */
  destroy() {
    document.removeEventListener('mousemove', this.handleMouseMove)
    document.removeEventListener('keydown', this.handleKeyDown)
    document.removeEventListener('fullscreenchange', this.handleFullscreenChange)
    document.removeEventListener('webkitfullscreenchange', this.handleFullscreenChange)
    document.removeEventListener('mozfullscreenchange', this.handleFullscreenChange)
    document.removeEventListener('MSFullscreenChange', this.handleFullscreenChange)
    window.removeEventListener('resize', this.handleWindowResize)
    
    const addressBar = document.querySelector('#omnibox-input, .location-bar-input')
    if (addressBar) {
      addressBar.removeEventListener('focus', this.handleAddressBarFocus)
      addressBar.removeEventListener('blur', this.handleAddressBarBlur)
    }
    
    this.cancelAutoHide()
    this.disableImmersiveMode()
  }
}

// Initialize immersive fullscreen event handler when DOM is ready
let immersiveHandler = null

document.addEventListener('DOMContentLoaded', () => {
  immersiveHandler = new ImmersiveFullscreenEventHandler()
  
  // Expose to global scope for C++ integration
  window.immersiveFullscreenHandler = immersiveHandler
})

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
  if (immersiveHandler) {
    immersiveHandler.destroy()
  }
})

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { ImmersiveFullscreenEventHandler }
}