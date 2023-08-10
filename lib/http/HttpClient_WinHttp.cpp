// Copyright (c) Microsoft Corporation. All rights reserved.
#include "mat/config.h"

#ifdef HAVE_MAT_DEFAULT_HTTP_CLIENT

#pragma warning(push)
#pragma warning(disable:4189)   /* Turn off Level 4: local variable is initialized but not referenced. dwError unused in Release without printing it. */
#include "HttpClient_WinHttp.hpp"
#include "utils/StringUtils.hpp"

#include <winhttp.h>

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>
#include <oacr.h>

namespace MAT_NS_BEGIN {

class WinHttpRequestWrapper
{
  protected:
    HttpClient_WinHttp&    m_parent;
    std::string            m_id;
    IHttpResponseCallback* m_appCallback {nullptr};
    HINTERNET              m_hWinHttpConnection {nullptr};
    HINTERNET              m_hWinHttpRequest {nullptr};
    SimpleHttpRequest*     m_request;
    SimpleHttpResponse*    m_response {nullptr};
    BYTE                   m_buffer[1024] {0};
    bool                   isCallbackCalled {false};
    bool                   isAborted {false};

  public:
    WinHttpRequestWrapper(HttpClient_WinHttp& parent, SimpleHttpRequest* request)
      : m_parent(parent),
        m_id(request->GetId()),
        m_request(request)
    {
        LOG_TRACE("%p WinHttpRequestWrapper()", this);
    }

    WinHttpRequestWrapper(WinHttpRequestWrapper const&) = delete;
    WinHttpRequestWrapper& operator=(WinHttpRequestWrapper const&) = delete;

    ~WinHttpRequestWrapper() noexcept
    {
        LOG_TRACE("%p ~WinHttpRequestWrapper()", this);
        if (m_hWinHttpRequest != nullptr)
        {
            ::WinHttpCloseHandle(m_hWinHttpRequest);
        }

        if (m_hWinHttpConnection != nullptr)
        {
            ::WinHttpCloseHandle(m_hWinHttpConnection);
        }
    }

    void Cancel()
    {
        LOCKGUARD(m_parent.m_requestsMutex);
        isAborted = true;
        if (m_hWinHttpRequest != nullptr)
        {
            ::WinHttpCloseHandle(m_hWinHttpRequest);
        }
    }

    void Send(IHttpResponseCallback* callback)
    {
        if (m_parent.m_hInternet == nullptr)
        {
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        LOCKGUARD(m_parent.m_requestsMutex);
        m_appCallback = callback;
        m_parent.m_requests[m_id] = this;

        if (isAborted)
        {
            // Request force-aborted before creating a WinInet handle.
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        DispatchEvent(OnConnecting);

        URL_COMPONENTS parts = {};
        parts.dwStructSize = sizeof(parts);
        parts.dwSchemeLength = static_cast<DWORD>(-1L);
        parts.dwHostNameLength = static_cast<DWORD>(-1L);
        parts.dwUrlPathLength = static_cast<DWORD>(-1L);
        parts.dwExtraInfoLength = static_cast<DWORD>(-1L);

        std::wstring url = UTF8ToWide(m_request->m_url);
        if (!::WinHttpCrackUrl(url.c_str(), 0, 0, &parts))
        {
            DWORD dwError = ::GetLastError();
            LOG_WARN("WinHttpCrackUrl() failed: dwError=%d url=%s", dwError, m_request->m_url.data());
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        std::wstring host(parts.lpszHostName, parts.lpszHostName + parts.dwHostNameLength);
        std::wstring path(parts.lpszUrlPath, parts.lpszUrlPath + parts.dwUrlPathLength);
        std::wstring extra(parts.lpszExtraInfo, parts.lpszExtraInfo + parts.dwExtraInfoLength);
        std::wstring urlPath = (path + extra);

        m_hWinHttpConnection = ::WinHttpConnect(
            m_parent.m_hInternet,
            host.c_str(),
            parts.nPort == INTERNET_DEFAULT_PORT ? INTERNET_DEFAULT_HTTPS_PORT : parts.nPort,
            0
        );

        if (m_hWinHttpConnection == nullptr)
        {
            DWORD dwError = ::GetLastError();
            LOG_WARN("WinHttpConnect() failed: %d", dwError);
            // Cannot connect to host
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        std::wstring verb = UTF8ToWide(m_request->m_method);
        m_hWinHttpRequest = ::WinHttpOpenRequest(
            m_hWinHttpConnection,
            verb.c_str(),
            urlPath.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (m_hWinHttpRequest == nullptr) {
            DWORD dwError = ::GetLastError();
            LOG_WARN("WinHttpOpenRequest() failed: %d", dwError);
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        ::WinHttpSetStatusCallback(m_hWinHttpRequest, WinHttpStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL);

        std::multimap<std::wstring, std::wstring> headerStrW;
        for (auto const& header : m_request->m_headers) {
            headerStrW.emplace(std::make_pair(UTF8ToWide(header.first), UTF8ToWide(header.second)));
        }

        std::vector<WINHTTP_EXTENDED_HEADER> headers;
        for (auto it = headerStrW.begin(); it != headerStrW.end(); ++it)
        {
            headers.emplace_back(
                WINHTTP_EXTENDED_HEADER{it->first.c_str(), it->second.c_str()}
            );
        }

        auto err = ::WinHttpAddRequestHeadersEx(
            m_hWinHttpRequest,
            WINHTTP_ADDREQ_FLAG_ADD,
            WINHTTP_EXTENDED_HEADER_FLAG_UNICODE,
            0,
            static_cast<DWORD>(headers.size()),
            headers.data()
        );

        if (err != ERROR_SUCCESS)
        {
            LOG_WARN("WinHttpAddRequestHeadersEx() failed: %d", err);
            DispatchEvent(OnConnectFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }

        DispatchEvent(OnSending);
        void* data = static_cast<void*>(m_request->m_body.data());
        DWORD size = static_cast<DWORD>(m_request->m_body.size());
        BOOL bResult = ::WinHttpSendRequest(
            m_hWinHttpRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            data,
            size,
            size,
            reinterpret_cast<DWORD_PTR>(this)
        );

        if (bResult == false) {
            DWORD dwError = ::GetLastError();
            LOG_WARN("WinHttpSendRequest() failed: %d", dwError);
            DispatchEvent(OnSendFailed);
            OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
            return;
        }
    }

    static void CALLBACK WinHttpStatusCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID statusInfo, DWORD statusInfoLength)
    {
        UNREFERENCED_PARAMETER(statusInfoLength);

        WinHttpRequestWrapper* self = reinterpret_cast<WinHttpRequestWrapper*>(context);
        LOG_TRACE("WinHttpStatusCallback: hInternet %p, dwContext %p, dwInternetStatus %u", handle, context, status);
        assert(self != nullptr);

        switch (status)
        {
            case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
            {
                assert(handle == self->m_hWinHttpRequest);
                return;
            }

            case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            {
                self->m_response = new SimpleHttpResponse(self->m_id);
                if(!::WinHttpReceiveResponse(handle, nullptr))
                {
                    DWORD dwError = ::GetLastError();
                    LOG_WARN("WinHttpReceiveResponse() failed: %d", dwError);
                    self->OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
                    return;
                }

                return;
            }

            case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
            {
                uint32_t statusCode;
                DWORD size = sizeof(statusCode);

                auto result = WinHttpQueryHeaders(
                    handle,
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                    nullptr,
                    &statusCode,
                    &size,
                    nullptr
                );

                if (result)
                {
                    self->m_response->m_statusCode = statusCode;
                }

                return;
            }

            case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            {
                DWORD headerSize = 0;

                auto result = WinHttpQueryHeaders(
                    handle,
                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    WINHTTP_NO_OUTPUT_BUFFER,
                    &headerSize,
                    WINHTTP_NO_HEADER_INDEX
                );

                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    DWORD bufferSize = headerSize / sizeof(wchar_t);
                    auto buffer = new wchar_t[bufferSize];
                    result = WinHttpQueryHeaders(
                        handle,
                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        buffer,
                        &headerSize,
                        WINHTTP_NO_HEADER_INDEX
                    );

                    if (result)
                    {
                        // Remove first line with status info.
                        // We're left with the key/value pairs.
                        const wchar_t* ptr = wcsstr(buffer, L"\r\n") + 2;
                        while (ptr < (buffer + bufferSize))
                        {
                            const wchar_t* colon = wcschr(ptr, L':');
                            if (!colon) {
                                break;
                            }

                            std::wstring key(ptr, colon);
                            ptr = colon + 1;
                            while (*ptr == L' ') {
                                ptr++;
                            }

                            const wchar_t* eol = wcsstr(ptr, L"\r\n");
                            if (!eol) {
                                break;
                            }

                            std::wstring value(ptr, eol);
                            self->m_response->m_headers.add(WideToUTF8(key), WideToUTF8(value));
                            ptr = eol + 2;
                        }
                    }
                    else
                    {
                        delete[] buffer;
                    }

                    if (!::WinHttpQueryDataAvailable(handle, NULL))
                    {
                        DWORD dwError = ::GetLastError();
                        LOG_WARN("WinHttpQueryDataAvailable() failed: %d", dwError);
                        self->OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
                    }
                }

                return;
            }

            case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            {
                if (self->m_response != nullptr)
                {
                    auto bytesAvailable = *reinterpret_cast<DWORD*>(statusInfo);
                    if (bytesAvailable > 0)
                    {
                        self->m_response->m_body.resize(bytesAvailable);
                        if(!::WinHttpReadData(handle, self->m_response->m_body.data(), bytesAvailable, NULL))
                        {
                            DWORD dwError = ::GetLastError();
                            LOG_WARN("WinHttpReadData() failed: %d", dwError);
                            self->OnRequestComplete(ERROR_WINHTTP_OPERATION_CANCELLED);
                            return;
                        }
                    }
                }

                return;
            }

            case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            {
                self->OnRequestComplete(ERROR_SUCCESS);
                return;
            }

            default:
                return;
        }
    }

    void DispatchEvent(HttpStateEvent type)
    {
        if (m_appCallback != nullptr)
        {
            m_appCallback->OnHttpStateEvent(type, static_cast<void*>(m_hWinHttpRequest), 0);
        }
    }

    void OnRequestComplete(DWORD dwError)
    {
        if (m_response == nullptr)
        {
            m_response = new SimpleHttpResponse(m_id);
        }

        if (dwError == ERROR_SUCCESS)
        {
            m_response->m_result = HttpResult_OK;
            DispatchEvent(OnResponse);
        }
        else
        {
             switch (dwError) {
                case ERROR_WINHTTP_OPERATION_CANCELLED:
                    m_response->m_result = HttpResult_Aborted;
                    break;

                case ERROR_WINHTTP_TIMEOUT:
                case ERROR_WINHTTP_NAME_NOT_RESOLVED:
                case ERROR_WINHTTP_CANNOT_CONNECT:
                    m_response->m_result = HttpResult_NetworkFailure;
                    break;

                default:
                    m_response->m_result = HttpResult_LocalFailure;
                    break;
            }
        }

        assert(isCallbackCalled == false);
        if (!isCallbackCalled)
        {
            ::WinHttpSetStatusCallback(m_hWinHttpRequest, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL);
            isCallbackCalled = true;
            m_appCallback->OnHttpResponse(m_response);
            m_parent.erase(m_id);
        }
    }

    static std::wstring UTF8ToWide(std::string str)
    {
        auto s = str.c_str();
        const size_t slen = str.length();
        const int len = MultiByteToWideChar(CP_UTF8, 0, s, static_cast<int>(slen), nullptr, 0);
        std::wstring result;
        result.resize(len);
        MultiByteToWideChar(CP_UTF8, 0, s, slen, (LPWSTR)result.data(), len);
        return result;
    }

    static std::string WideToUTF8(std::wstring wstr)
    {
        auto s = wstr.c_str();
        const size_t slen = wstr.length();
        const int len = WideCharToMultiByte(CP_UTF8, 0, s, static_cast<int>(slen), nullptr, 0, nullptr, nullptr);
        std::string result;
        result.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, s, slen, (LPSTR)result.data(), len, nullptr, nullptr);
        return result;
    }
};

unsigned HttpClient_WinHttp::s_nextRequestId = 0;

HttpClient_WinHttp::HttpClient_WinHttp()
{
    m_hInternet = ::WinHttpOpen(
        L"SimpleWinHttp Sample/1.0",                                        // HTTP UserAgent value
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,                                // Use System and per-User proxy settings
        WINHTTP_NO_PROXY_NAME,                                              // Empty value
        WINHTTP_NO_PROXY_BYPASS,                                            // Empty value
        WINHTTP_FLAG_ASYNC | WINHTTP_FLAG_SECURE_DEFAULTS                   // Use HTTPS and asynchronous handling
    );
}

HttpClient_WinHttp::~HttpClient_WinHttp()
{
    CancelAllRequests();
    if (m_hInternet != nullptr)
    {
        ::WinHttpCloseHandle(m_hInternet);
    }
}

void HttpClient_WinHttp::erase(std::string const& id)
{
    LOCKGUARD(m_requestsMutex);
    auto it = m_requests.find(id);
    if (it != m_requests.end()) {
        auto req = it->second;
        m_requests.erase(it);
        delete req;
    }
}

IHttpRequest* HttpClient_WinHttp::CreateRequest()
{
    std::string id = "WI-" + toString(::InterlockedIncrement(&s_nextRequestId));
    return new SimpleHttpRequest(id);
}

void HttpClient_WinHttp::SendRequestAsync(IHttpRequest* request, IHttpResponseCallback* callback)
{
    // Note: 'request' is never owned by IHttpClient and gets deleted in EventsUploadContext.clear()
    WinHttpRequestWrapper *wrapper = new WinHttpRequestWrapper(*this, static_cast<SimpleHttpRequest*>(request));
    wrapper->Send(callback);
}

void HttpClient_WinHttp::CancelRequestAsync(std::string const& id)
{
    LOCKGUARD(m_requestsMutex);
    auto it = m_requests.find(id);
    if (it != m_requests.end()) {
        auto request = it->second;
        if (request) {
            request->Cancel();
        }
    }
}

void HttpClient_WinHttp::CancelAllRequests()
{
    // vector of all request IDs
    std::vector<std::string> ids;
    {
        LOCKGUARD(m_requestsMutex);
        for (auto const& item : m_requests) {
            ids.push_back(item.first);
        }
    }
    // cancel all requests one-by-one not holding the lock
    for (const auto &id : ids)
        CancelRequestAsync(id);

    // wait for all destructors to run
    while (!m_requests.empty())
    {
        PAL::sleep(100);
        std::this_thread::yield();
    }
};

} MAT_NS_END
#pragma warning(pop)
#endif // HAVE_MAT_DEFAULT_HTTP_CLIENT
// clang-format on
