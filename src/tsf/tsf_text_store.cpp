// ============================================================================
// tsf_text_store.cpp - TSF Text Store Implementation
// ============================================================================
//
// Implements ITextStoreACP and ITfContextOwnerCompositionSink to bridge
// between TSF (Text Services Framework) and TextInput widgets.
//
// Key design decision: TsfTextStore holds a non-owning pointer to TextInput
// because widget lifetime is managed by arena allocator, not COM ref counting.
// The widget must call tsf_destroy_text_store() before being destroyed.
//
// ============================================================================

#include "../../include/tsf_types.h"
#include <olectl.h>  // For CONNECT_E_ADVISELIMIT, CONNECT_E_NOCONNECTION

// Forward declaration of TextInput (defined in ui_widgets_basic.cpp)
struct TextInput;

// Access TextInput fields (TextInput is defined later in unity build)
// These functions are defined in ui_widgets_basic.cpp after TextInput struct
const wchar_t* textinput_get_text(TextInput* input);
size_t textinput_get_text_len(TextInput* input);
void textinput_get_selection(TextInput* input, size_t* start, size_t* end);
void textinput_set_selection(TextInput* input, size_t start, size_t end);
void textinput_set_text_range(TextInput* input, size_t start, size_t end, const wchar_t* text, size_t len);
void textinput_get_rect(TextInput* input, float* x, float* y, float* width, float* height);
HWND textinput_get_hwnd();

// ============================================================================
// TsfTextStore - COM Implementation
// ============================================================================

struct TsfTextStore : public ITextStoreACP, public ITfContextOwnerCompositionSink {
    // Non-owning back-pointer to widget (widget lifetime > TsfTextStore)
    TextInput* owner_ = nullptr;

    // TSF document/context management
    ITfDocumentMgr* document_mgr_ = nullptr;
    ITfContext* context_ = nullptr;

    // TSF sink for notifications
    ITextStoreACPSink* sink_ = nullptr;
    DWORD sink_mask_ = 0;

    // Lock state
    DWORD lock_type_ = 0;

    // Composition state
    bool composition_active_ = false;
    LONG composition_start_ = 0;
    LONG composition_end_ = 0;

    // Reference count (COM requirement, but we don't delete on zero)
    LONG ref_count_ = 1;

    // ========================================================================
    // Construction / Destruction
    // ========================================================================

    TsfTextStore(TextInput* owner) : owner_(owner) {
        if (!g_tsf.initialized || !g_tsf.thread_mgr) {
            return;
        }

        // Create document manager
        HRESULT hr = g_tsf.thread_mgr->CreateDocumentMgr(&document_mgr_);
        if (FAILED(hr) || !document_mgr_) {
            LOG_WARN("TSF: Failed to create document manager");
            return;
        }

        // Create context with this text store
        TfEditCookie edit_cookie = 0;
        hr = document_mgr_->CreateContext(
            g_tsf.client_id,
            0,  // flags
            static_cast<ITextStoreACP*>(this),
            &context_,
            &edit_cookie
        );

        if (FAILED(hr) || !context_) {
            LOG_WARN("TSF: Failed to create context");
            document_mgr_->Release();
            document_mgr_ = nullptr;
            return;
        }

        // Push context to document manager stack
        hr = document_mgr_->Push(context_);
        if (FAILED(hr)) {
            LOG_WARN("TSF: Failed to push context");
            context_->Release();
            context_ = nullptr;
            document_mgr_->Release();
            document_mgr_ = nullptr;
            return;
        }
    }

    ~TsfTextStore() {
        if (context_) {
            if (document_mgr_) {
                document_mgr_->Pop(TF_POPF_ALL);
            }
            context_->Release();
        }
        if (document_mgr_) {
            document_mgr_->Release();
        }
        if (sink_) {
            sink_->Release();
        }
    }

    // ========================================================================
    // Focus Management
    // ========================================================================

    void OnFocus() {
        if (document_mgr_ && g_tsf.thread_mgr) {
            g_tsf.thread_mgr->SetFocus(document_mgr_);
        }
    }

    void OnBlur() {
        // End any active composition before losing focus
        if (composition_active_ && sink_) {
            // Notify TSF that composition is being terminated
            composition_active_ = false;
            composition_start_ = 0;
            composition_end_ = 0;
        }

        if (g_tsf.thread_mgr) {
            g_tsf.thread_mgr->SetFocus(nullptr);
        }
    }

    // ========================================================================
    // IUnknown Implementation
    // ========================================================================

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;

        *ppv = nullptr;

        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_ITextStoreACP)) {
            *ppv = static_cast<ITextStoreACP*>(this);
        }
        else if (IsEqualIID(riid, IID_ITfContextOwnerCompositionSink)) {
            *ppv = static_cast<ITfContextOwnerCompositionSink*>(this);
        }
        else {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&ref_count_);
    }

    STDMETHODIMP_(ULONG) Release() override {
        LONG count = InterlockedDecrement(&ref_count_);
        // Note: We don't delete on zero - widget owns our lifetime
        return count;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Sink Management
    // ========================================================================

    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask) override {
        if (!IsEqualIID(riid, IID_ITextStoreACPSink)) {
            return E_INVALIDARG;
        }

        if (!punk) {
            return E_INVALIDARG;
        }

        // If we already have a sink, check if it's the same one
        if (sink_) {
            if (sink_ == punk) {
                // Same sink, just update mask
                sink_mask_ = dwMask;
                return S_OK;
            }
            // Different sink - TSF should call UnadviseSink first
            return CONNECT_E_ADVISELIMIT;
        }

        // Query for ITextStoreACPSink
        HRESULT hr = punk->QueryInterface(IID_ITextStoreACPSink, reinterpret_cast<void**>(&sink_));
        if (FAILED(hr)) {
            return E_INVALIDARG;
        }

        sink_mask_ = dwMask;
        return S_OK;
    }

    STDMETHODIMP UnadviseSink(IUnknown* punk) override {
        if (!sink_ || sink_ != punk) {
            return CONNECT_E_NOCONNECTION;
        }

        sink_->Release();
        sink_ = nullptr;
        sink_mask_ = 0;
        return S_OK;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Lock Management
    // ========================================================================

    STDMETHODIMP RequestLock(DWORD dwLockFlags, HRESULT* phrSession) override {
        if (!phrSession) return E_INVALIDARG;
        if (!sink_) return E_UNEXPECTED;

        // Check for lock reentrancy
        if (lock_type_ != 0) {
            // Already have a lock - check if upgrade is possible
            if ((lock_type_ & TS_LF_READWRITE) == TS_LF_READ &&
                (dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE) {
                // Can't upgrade read to read-write
                *phrSession = TS_E_SYNCHRONOUS;
                return S_OK;
            }
            // Allow nested read locks
            if ((dwLockFlags & TS_LF_READWRITE) == TS_LF_READ) {
                lock_type_ |= dwLockFlags;
                *phrSession = sink_->OnLockGranted(dwLockFlags);
                return S_OK;
            }
            *phrSession = TS_E_SYNCHRONOUS;
            return S_OK;
        }

        // Grant the lock synchronously (we're single-threaded UI)
        lock_type_ = dwLockFlags;
        *phrSession = sink_->OnLockGranted(dwLockFlags);
        lock_type_ = 0;

        return S_OK;
    }

    STDMETHODIMP GetStatus(TS_STATUS* pdcs) override {
        if (!pdcs) return E_INVALIDARG;

        pdcs->dwDynamicFlags = 0;
        pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT;  // No hidden text

        return S_OK;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Text Access
    // ========================================================================

    STDMETHODIMP GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq,
                         ULONG* pcchPlainRet, TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq,
                         ULONG* pcRunInfoRet, LONG* pacpNext) override {
        if (!owner_) return E_UNEXPECTED;
        if (!pcchPlainRet || !pacpNext) return E_INVALIDARG;

        const wchar_t* text = textinput_get_text(owner_);
        size_t text_len = textinput_get_text_len(owner_);

        // Validate range
        if (acpStart < 0) acpStart = 0;
        if (acpEnd < 0 || acpEnd > static_cast<LONG>(text_len)) {
            acpEnd = static_cast<LONG>(text_len);
        }
        if (acpStart > acpEnd) acpStart = acpEnd;

        LONG copy_len = acpEnd - acpStart;

        // Copy text if buffer provided
        if (pchPlain && cchPlainReq > 0) {
            ULONG to_copy = (copy_len < static_cast<LONG>(cchPlainReq)) ? copy_len : cchPlainReq - 1;
            memcpy(pchPlain, text + acpStart, to_copy * sizeof(wchar_t));
            *pcchPlainRet = to_copy;
        } else {
            *pcchPlainRet = 0;
        }

        // Provide run info (plain text = one run)
        if (prgRunInfo && cRunInfoReq > 0 && pcRunInfoRet) {
            prgRunInfo[0].type = TS_RT_PLAIN;
            prgRunInfo[0].uCount = copy_len;
            *pcRunInfoRet = 1;
        } else if (pcRunInfoRet) {
            *pcRunInfoRet = 0;
        }

        *pacpNext = acpEnd;
        return S_OK;
    }

    STDMETHODIMP SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd,
                         const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange) override {
        (void)dwFlags;
        if (!owner_) return E_UNEXPECTED;
        if (!(lock_type_ & TS_LF_READWRITE)) return TS_E_NOLOCK;

        size_t text_len = textinput_get_text_len(owner_);

        // Validate range
        if (acpStart < 0) acpStart = 0;
        if (acpEnd < 0 || acpEnd > static_cast<LONG>(text_len)) {
            acpEnd = static_cast<LONG>(text_len);
        }
        if (acpStart > acpEnd) acpStart = acpEnd;

        // Apply the change
        textinput_set_text_range(owner_, acpStart, acpEnd, pchText, cch);

        // Report the change
        if (pChange) {
            pChange->acpStart = acpStart;
            pChange->acpOldEnd = acpEnd;
            pChange->acpNewEnd = acpStart + static_cast<LONG>(cch);
        }

        return S_OK;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Selection
    // ========================================================================

    STDMETHODIMP GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection,
                              ULONG* pcFetched) override {
        if (!owner_) return E_UNEXPECTED;
        if (!pSelection || !pcFetched) return E_INVALIDARG;
        if (ulIndex != 0 && ulIndex != TF_DEFAULT_SELECTION) return E_INVALIDARG;
        if (ulCount == 0) {
            *pcFetched = 0;
            return S_OK;
        }

        size_t sel_start, sel_end;
        textinput_get_selection(owner_, &sel_start, &sel_end);

        pSelection[0].acpStart = static_cast<LONG>(sel_start < sel_end ? sel_start : sel_end);
        pSelection[0].acpEnd = static_cast<LONG>(sel_start > sel_end ? sel_start : sel_end);
        pSelection[0].style.ase = TS_AE_END;  // Active end is at the end
        pSelection[0].style.fInterimChar = FALSE;

        *pcFetched = 1;
        return S_OK;
    }

    STDMETHODIMP SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection) override {
        if (!owner_) return E_UNEXPECTED;
        if (!(lock_type_ & TS_LF_READWRITE)) return TS_E_NOLOCK;
        if (ulCount != 1 || !pSelection) return E_INVALIDARG;

        textinput_set_selection(owner_, pSelection[0].acpStart, pSelection[0].acpEnd);
        return S_OK;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Text Extent (for IME window positioning)
    // ========================================================================

    STDMETHODIMP GetTextExt(TfEditCookie ec, LONG acpStart, LONG acpEnd,
                            RECT* prc, BOOL* pfClipped) override {
        (void)ec;
        if (!owner_) return E_UNEXPECTED;
        if (!prc || !pfClipped) return E_INVALIDARG;

        // Get widget rect in screen coordinates
        float wx, wy, ww, wh;
        textinput_get_rect(owner_, &wx, &wy, &ww, &wh);

        HWND hwnd = textinput_get_hwnd();
        if (!hwnd) return E_UNEXPECTED;

        // Get cursor position using DirectWrite hit testing
        // For now, return a simple rect based on widget position
        // A full implementation would use HitTestTextPosition()

        POINT pt = { static_cast<LONG>(wx), static_cast<LONG>(wy) };
        ClientToScreen(hwnd, &pt);

        // Default: return rect at start of text area
        prc->left = pt.x + 8;  // padding_left
        prc->top = pt.y + 6;   // padding_top
        prc->right = prc->left + 2;  // cursor width
        prc->bottom = pt.y + static_cast<LONG>(wh) - 6;  // padding_bottom

        // If we have specific positions, calculate them
        if (acpStart >= 0 && g_dwrite_factory && owner_) {
            const wchar_t* text = textinput_get_text(owner_);
            size_t text_len = textinput_get_text_len(owner_);

            if (text_len > 0) {
                IDWriteTextLayout* layout = nullptr;
                HRESULT hr = g_dwrite_factory->CreateTextLayout(
                    text, static_cast<UINT32>(text_len),
                    g_text_format_body,
                    ww - 16.0f,  // padding left + right
                    wh - 12.0f,  // padding top + bottom
                    &layout
                );

                if (SUCCEEDED(hr) && layout) {
                    DWRITE_HIT_TEST_METRICS metrics;
                    float x1, y1;

                    // Get start position
                    hr = layout->HitTestTextPosition(
                        static_cast<UINT32>(acpStart < static_cast<LONG>(text_len) ? acpStart : text_len - 1),
                        FALSE, &x1, &y1, &metrics
                    );

                    if (SUCCEEDED(hr)) {
                        prc->left = pt.x + 8 + static_cast<LONG>(x1);

                        // Get end position
                        if (acpEnd > acpStart && acpEnd <= static_cast<LONG>(text_len)) {
                            float x2, y2;
                            hr = layout->HitTestTextPosition(
                                static_cast<UINT32>(acpEnd - 1),
                                TRUE, &x2, &y2, &metrics
                            );
                            if (SUCCEEDED(hr)) {
                                prc->right = pt.x + 8 + static_cast<LONG>(x2);
                            }
                        } else {
                            prc->right = prc->left + 2;
                        }
                    }

                    layout->Release();
                }
            }
        }

        *pfClipped = FALSE;
        return S_OK;
    }

    STDMETHODIMP GetScreenExt(TfEditCookie ec, RECT* prc) override {
        (void)ec;
        if (!prc) return E_INVALIDARG;
        if (!owner_) return E_UNEXPECTED;

        float wx, wy, ww, wh;
        textinput_get_rect(owner_, &wx, &wy, &ww, &wh);

        HWND hwnd = textinput_get_hwnd();
        if (!hwnd) return E_UNEXPECTED;

        POINT pt = { static_cast<LONG>(wx), static_cast<LONG>(wy) };
        ClientToScreen(hwnd, &pt);

        prc->left = pt.x;
        prc->top = pt.y;
        prc->right = pt.x + static_cast<LONG>(ww);
        prc->bottom = pt.y + static_cast<LONG>(wh);

        return S_OK;
    }

    STDMETHODIMP GetWnd(TfEditCookie ec, HWND* phwnd) override {
        (void)ec;
        if (!phwnd) return E_INVALIDARG;
        *phwnd = textinput_get_hwnd();
        return (*phwnd) ? S_OK : E_UNEXPECTED;
    }

    // ========================================================================
    // ITextStoreACP Implementation - Misc Methods
    // ========================================================================

    STDMETHODIMP GetEndACP(LONG* pacp) override {
        if (!pacp) return E_INVALIDARG;
        if (!owner_) return E_UNEXPECTED;
        *pacp = static_cast<LONG>(textinput_get_text_len(owner_));
        return S_OK;
    }

    STDMETHODIMP GetActiveView(TsViewCookie* pvcView) override {
        if (!pvcView) return E_INVALIDARG;
        *pvcView = 1;  // We only have one view
        return S_OK;
    }

    STDMETHODIMP GetACPFromPoint(TsViewCookie vcView, const POINT* pt,
                                  DWORD dwFlags, LONG* pacp) override {
        (void)vcView; (void)pt; (void)dwFlags;
        if (!pacp) return E_INVALIDARG;
        // Not implemented - return start of text
        *pacp = 0;
        return S_OK;
    }

    STDMETHODIMP QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch,
                             LONG* pacpResultStart, LONG* pacpResultEnd) override {
        if (!pacpResultStart || !pacpResultEnd) return E_INVALIDARG;

        // Allow insertion at any valid position
        size_t text_len = owner_ ? textinput_get_text_len(owner_) : 0;

        if (acpTestStart < 0) acpTestStart = 0;
        if (acpTestEnd < 0 || acpTestEnd > static_cast<LONG>(text_len)) {
            acpTestEnd = static_cast<LONG>(text_len);
        }
        if (acpTestStart > acpTestEnd) acpTestStart = acpTestEnd;

        *pacpResultStart = acpTestStart;
        *pacpResultEnd = acpTestStart + static_cast<LONG>(cch);

        return S_OK;
    }

    STDMETHODIMP InsertTextAtSelection(DWORD dwFlags, const WCHAR* pchText, ULONG cch,
                                        LONG* pacpStart, LONG* pacpEnd,
                                        TS_TEXTCHANGE* pChange) override {
        if (!owner_) return E_UNEXPECTED;

        size_t sel_start, sel_end;
        textinput_get_selection(owner_, &sel_start, &sel_end);

        LONG acpStart = static_cast<LONG>(sel_start < sel_end ? sel_start : sel_end);
        LONG acpEnd = static_cast<LONG>(sel_start > sel_end ? sel_start : sel_end);

        if (dwFlags & TS_IAS_QUERYONLY) {
            // Query only - don't modify
            if (pacpStart) *pacpStart = acpStart;
            if (pacpEnd) *pacpEnd = acpEnd;
            return S_OK;
        }

        // Insert the text
        textinput_set_text_range(owner_, acpStart, acpEnd, pchText, cch);

        if (pacpStart) *pacpStart = acpStart;
        if (pacpEnd) *pacpEnd = acpStart + static_cast<LONG>(cch);

        if (pChange) {
            pChange->acpStart = acpStart;
            pChange->acpOldEnd = acpEnd;
            pChange->acpNewEnd = acpStart + static_cast<LONG>(cch);
        }

        if (!(dwFlags & TS_IAS_NOQUERY)) {
            // Update selection to end of inserted text
            textinput_set_selection(owner_, acpStart + cch, acpStart + cch);
        }

        return S_OK;
    }

    STDMETHODIMP GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject** ppDataObject) override {
        (void)acpStart; (void)acpEnd;
        if (!ppDataObject) return E_INVALIDARG;
        *ppDataObject = nullptr;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetEmbedded(LONG acpPos, REFGUID rguidService,
                              REFIID riid, IUnknown** ppunk) override {
        (void)acpPos; (void)rguidService; (void)riid;
        if (!ppunk) return E_INVALIDARG;
        *ppunk = nullptr;
        return E_NOTIMPL;
    }

    STDMETHODIMP InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd,
                                 IDataObject* pDataObject, TS_TEXTCHANGE* pChange) override {
        (void)dwFlags; (void)acpStart; (void)acpEnd; (void)pDataObject; (void)pChange;
        return E_NOTIMPL;
    }

    STDMETHODIMP QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc,
                                      BOOL* pfInsertable) override {
        (void)pguidService; (void)pFormatEtc;
        if (!pfInsertable) return E_INVALIDARG;
        *pfInsertable = FALSE;  // We don't support embedded objects
        return S_OK;
    }

    STDMETHODIMP InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject* pDataObject,
                                            LONG* pacpStart, LONG* pacpEnd,
                                            TS_TEXTCHANGE* pChange) override {
        (void)dwFlags; (void)pDataObject; (void)pacpStart; (void)pacpEnd; (void)pChange;
        return E_NOTIMPL;  // We don't support embedded objects
    }

    STDMETHODIMP RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs,
                                        const TS_ATTRID* paFilterAttrs) override {
        (void)dwFlags; (void)cFilterAttrs; (void)paFilterAttrs;
        return S_OK;
    }

    STDMETHODIMP RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs,
                                         const TS_ATTRID* paFilterAttrs, DWORD dwFlags) override {
        (void)acpPos; (void)cFilterAttrs; (void)paFilterAttrs; (void)dwFlags;
        return S_OK;
    }

    STDMETHODIMP RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs,
                                                      const TS_ATTRID* paFilterAttrs,
                                                      DWORD dwFlags) override {
        (void)acpPos; (void)cFilterAttrs; (void)paFilterAttrs; (void)dwFlags;
        return S_OK;
    }

    STDMETHODIMP FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
                                         const TS_ATTRID* paFilterAttrs, DWORD dwFlags,
                                         LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset) override {
        (void)acpStart; (void)acpHalt; (void)cFilterAttrs;
        (void)paFilterAttrs; (void)dwFlags;
        if (!pacpNext || !pfFound || !plFoundOffset) return E_INVALIDARG;
        *pacpNext = 0;
        *pfFound = FALSE;
        *plFoundOffset = 0;
        return S_OK;
    }

    STDMETHODIMP RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals,
                                         ULONG* pcFetched) override {
        (void)ulCount; (void)paAttrVals;
        if (!pcFetched) return E_INVALIDARG;
        *pcFetched = 0;
        return S_OK;
    }

    // ========================================================================
    // ITfContextOwnerCompositionSink Implementation
    // ========================================================================

    STDMETHODIMP OnStartComposition(ITfCompositionView* pComposition, BOOL* pfOk) override {
        (void)pComposition;
        if (!pfOk) return E_INVALIDARG;

        composition_active_ = true;

        // Get initial composition range
        if (owner_) {
            size_t sel_start, sel_end;
            textinput_get_selection(owner_, &sel_start, &sel_end);
            composition_start_ = static_cast<LONG>(sel_start < sel_end ? sel_start : sel_end);
            composition_end_ = composition_start_;
        }

        *pfOk = TRUE;
        return S_OK;
    }

    STDMETHODIMP OnUpdateComposition(ITfCompositionView* pComposition,
                                      ITfRange* pRangeNew) override {
        (void)pComposition;

        if (pRangeNew && owner_) {
            // Query the range for ACP positions
            ITfRangeACP* range_acp = nullptr;
            HRESULT hr = pRangeNew->QueryInterface(IID_ITfRangeACP,
                                                    reinterpret_cast<void**>(&range_acp));
            if (SUCCEEDED(hr) && range_acp) {
                LONG start, length;
                hr = range_acp->GetExtent(&start, &length);
                if (SUCCEEDED(hr)) {
                    composition_start_ = start;
                    composition_end_ = start + length;
                }
                range_acp->Release();
            }
        }

        return S_OK;
    }

    STDMETHODIMP OnEndComposition(ITfCompositionView* pComposition) override {
        (void)pComposition;

        composition_active_ = false;
        composition_start_ = 0;
        composition_end_ = 0;

        return S_OK;
    }
};

// ============================================================================
// Factory Functions
// ============================================================================

TsfTextStore* tsf_create_text_store(TextInput* owner) {
    if (!tsf_is_initialized()) {
        return nullptr;
    }
    return new TsfTextStore(owner);
}

void tsf_destroy_text_store(TsfTextStore* store) {
    if (store) {
        delete store;
    }
}

void tsf_set_focus(TsfTextStore* store) {
    if (store) {
        store->OnFocus();
    }
}

void tsf_clear_focus(TsfTextStore* store) {
    if (store) {
        store->OnBlur();
    }
}

bool tsf_is_composing(TsfTextStore* store) {
    return store ? store->composition_active_ : false;
}

void tsf_get_composition_range(TsfTextStore* store, size_t* start, size_t* end) {
    if (!store || !start || !end) return;
    *start = static_cast<size_t>(store->composition_start_);
    *end = static_cast<size_t>(store->composition_end_);
}

void tsf_notify_text_change(TsfTextStore* store, size_t start, size_t old_end, size_t new_end) {
    if (!store || !store->sink_) return;
    if (!(store->sink_mask_ & TS_AS_TEXT_CHANGE)) return;

    TS_TEXTCHANGE change;
    change.acpStart = static_cast<LONG>(start);
    change.acpOldEnd = static_cast<LONG>(old_end);
    change.acpNewEnd = static_cast<LONG>(new_end);

    store->sink_->OnTextChange(0, &change);
}

void tsf_notify_selection_change(TsfTextStore* store) {
    if (!store || !store->sink_) return;
    if (!(store->sink_mask_ & TS_AS_SEL_CHANGE)) return;

    store->sink_->OnSelectionChange();
}

// Note: TextInput accessor functions (textinput_get_text, etc.) are defined
// in ui_widgets_basic.cpp after the TextInput struct. They are forward-declared
// at the top of this file and resolved by the unity build.
