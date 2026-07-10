#pragma once

// ============================================================================
// TSF (Text Services Framework) Type Definitions
// ============================================================================
//
// Forward declarations and types for TSF integration. TSF enables:
// - East Asian IME input (Chinese, Japanese, Korean)
// - Composition preview with underline
// - Candidate window positioning
// - Touch keyboard integration
// - Speech-to-text input
//
// ============================================================================

#include <msctf.h>

// Forward declarations
struct TextInput;  // Defined in ui_widgets_basic.cpp

// TSF global state (single instance per application)
struct TsfGlobals {
    ITfThreadMgrEx* thread_mgr = nullptr;      // TSF thread manager
    TfClientId client_id = TF_CLIENTID_NULL;   // Client ID from activation
    bool initialized = false;                   // True if TSF is active
};

// TSF text store - COM object that bridges TextInput and TSF
// Forward declaration - full implementation in tsf_text_store.cpp
struct TsfTextStore;

// Global TSF state accessor (defined in tsf_globals.cpp)
extern TsfGlobals g_tsf;

// TSF initialization functions (called from platform.cpp)
bool tsf_init();
void tsf_cleanup();
bool tsf_is_initialized();

// TsfTextStore factory (called from TextInput constructor)
TsfTextStore* tsf_create_text_store(TextInput* owner);
void tsf_destroy_text_store(TsfTextStore* store);

// TSF focus management (called from TextInput on_focus/on_blur)
void tsf_set_focus(TsfTextStore* store);
void tsf_clear_focus(TsfTextStore* store);

// TSF composition state queries
bool tsf_is_composing(TsfTextStore* store);
void tsf_get_composition_range(TsfTextStore* store, size_t* start, size_t* end);

// TSF change notification (called from TextInput when text changes)
void tsf_notify_text_change(TsfTextStore* store, size_t start, size_t old_end, size_t new_end);
void tsf_notify_selection_change(TsfTextStore* store);
