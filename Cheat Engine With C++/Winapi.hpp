#pragma once 
#include <windows.h>
#include <sapi.h>
#include <string>
#include <objbase.h>
#include <comdef.h>
#include <comip.h>

// 常量声明 
const DWORD SPF_DEFAULT = 0;
const DWORD SPF_ASYNC = (1 << 0);
const DWORD SPF_PURGEBEFORESPEAK = (1 << 1);
const DWORD SPF_IS_FILENAME = (1 << 2);
const DWORD SPF_IS_XML = (1 << 3);
const DWORD SPF_IS_NOT_XML = (1 << 4);
const DWORD SPF_PERSIST_XML = (1 << 5);
const DWORD SPF_NLP_SPEAK_PUNC = (1 << 6);
const DWORD SPF_PARSE_SAPI = (1 << 7);
const DWORD SPF_PARSE_SSML = (1 << 8);
const DWORD SPF_PARSE_AUTODETECT = 0;

// GUID声明 
const CLSID CLSID_SpVoice = { 0x96749377, 0x3391, 0x11D2, {0x9E, 0xE3, 0x00, 0xC0, 0x4F, 0x79, 0x73, 0x96} };

// 接口声明 
struct ISpVoice : public ISpEventSource {
    virtual HRESULT STDMETHODCALLTYPE SetOutput(IUnknown* pUnkOutput, BOOL fAllowFormatChanges) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOutputObjectToken(ISpObjectToken** ppObjectToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetOutputStream(ISpStreamFormat** ppStream) = 0;
    virtual HRESULT STDMETHODCALLTYPE Pause() = 0;
    virtual HRESULT STDMETHODCALLTYPE Resume() = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVoice(ISpObjectToken* pToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVoice(ISpObjectToken** ppToken) = 0;
    virtual HRESULT STDMETHODCALLTYPE Speak(LPCWSTR pwcs, DWORD dwFlags, ULONG* pulStreamNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE SpeakStream(IStream* pStream, DWORD dwFlags, ULONG* pulStreamNumber) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetStatus(SPVOICESTATUS* pStatus, LPWSTR* ppszLastBookmark) = 0;
    virtual HRESULT STDMETHODCALLTYPE Skip(LPCWSTR pItemType, LONG lNumItems, ULONG* pulNumSkipped) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetPriority(SPVPRIORITY ePriority) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPriority(SPVPRIORITY* pePriority) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetAlertBoundary(SPEVENTENUM eBoundary) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAlertBoundary(SPEVENTENUM* peBoundary) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetRate(LONG RateAdjust) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRate(LONG* pRateAdjust) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetVolume(USHORT usVolume) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVolume(USHORT* pusVolume) = 0;
    virtual HRESULT STDMETHODCALLTYPE WaitUntilDone(ULONG msTimeout) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetSyncSpeakTimeout(ULONG msTimeout) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetSyncSpeakTimeout(ULONG* pmsTimeout) = 0;
    virtual HRESULT STDMETHODCALLTYPE SpeakCompleteEvent() = 0;
    virtual HRESULT STDMETHODCALLTYPE IsUISupported(LPCWSTR pszTypeOfUI, void* pvExtraData, ULONG cbExtraData, BOOL* pfSupported) = 0;
    virtual HRESULT STDMETHODCALLTYPE DisplayUI(HWND hwndParent, LPCWSTR pszTitle, LPCWSTR pszTypeOfUI, void* pvExtraData, ULONG cbExtraData) = 0;
};

namespace WinsAPI {
    HRESULT Speak(const std::wstring& s, DWORD flags);
    HRESULT Speak(const std::wstring& s, bool waitTillDone = false);

    // 全局语音实例 
    static ISpVoice* g_pVoice = nullptr;
    static bool g_bNoVoice = false;

    inline HRESULT Speak(const std::wstring& s, DWORD flags) {
        if (!g_pVoice && !g_bNoVoice) {
            HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void**)&g_pVoice);
            if (FAILED(hr)) {
                g_bNoVoice = true;
                return hr;
            }
        }

        if (g_pVoice) {
            return g_pVoice->Speak(s.c_str(), flags, nullptr);
        }
        return E_FAIL;
    }

    inline HRESULT Speak(const std::wstring& s, bool waitTillDone) {
        DWORD flags = SPF_PURGEBEFORESPEAK;
        if (!waitTillDone) {
            flags |= SPF_ASYNC;
        }
        return Speak(s, flags);
    }
}