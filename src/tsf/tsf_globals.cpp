// ============================================================================
// tsf_globals.cpp - TSF (Text Services Framework) Global Initialization
// ============================================================================
//
// Manages the global TSF thread manager and client registration.
// Call tsf_init() after CoInitializeEx() in app startup.
// Call tsf_cleanup() before CoUninitialize() in app shutdown.
//
// ============================================================================

#include "../../include/tsf_types.h"

// Global TSF state
TsfGlobals g_tsf;

// ============================================================================
// TSF Initialization
// ============================================================================

bool tsf_init() {
    if (g_tsf.initialized) {
        return true;  // Already initialized
    }

    // Create TSF thread manager
    HRESULT hr = CoCreateInstance(
        CLSID_TF_ThreadMgr,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_ITfThreadMgrEx,
        reinterpret_cast<void**>(&g_tsf.thread_mgr)
    );

    if (FAILED(hr) || !g_tsf.thread_mgr) {
        LOG_WARN("TSF: Failed to create thread manager (hr=0x%08X)", hr);
        return false;
    }

    // Activate as a TSF client
    hr = g_tsf.thread_mgr->ActivateEx(&g_tsf.client_id, TF_TMAE_UIELEMENTENABLEDONLY);
    if (FAILED(hr)) {
        LOG_WARN("TSF: Failed to activate client (hr=0x%08X)", hr);
        g_tsf.thread_mgr->Release();
        g_tsf.thread_mgr = nullptr;
        return false;
    }

    g_tsf.initialized = true;
    LOG_INFO("TSF: Initialized (client_id=%u)", g_tsf.client_id);
    return true;
}

void tsf_cleanup() {
    if (!g_tsf.initialized) {
        return;
    }

    if (g_tsf.thread_mgr) {
        g_tsf.thread_mgr->Deactivate();
        g_tsf.thread_mgr->Release();
        g_tsf.thread_mgr = nullptr;
    }

    g_tsf.client_id = TF_CLIENTID_NULL;
    g_tsf.initialized = false;
    LOG_INFO("TSF: Cleaned up");
}

bool tsf_is_initialized() {
    return g_tsf.initialized;
}
