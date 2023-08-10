//
// Copyright (c) 2015-2020 Microsoft Corporation and Contributors.
// SPDX-License-Identifier: Apache-2.0
//
#ifndef HTTPCLIENT_WINRT_HPP
#define HTTPCLIENT_WINRT_HPP

#ifdef HAVE_MAT_DEFAULT_HTTP_CLIENT

#pragma comment(lib, "windowsapp")
#pragma comment(lib, "runtimeobject")

#include <Windows.h>

#include "IHttpClient.hpp"
#include "pal/PAL.hpp"

#include <ppltasks.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Web.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Web.Http.Headers.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Web;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Web::Http::Filters;
using namespace winrt::Windows::Web::Http::Headers;

namespace MAT_NS_BEGIN {

class WinRtRequestWrapper;

class HttpClient_WinRt : public IHttpClient {
  public:
    HttpClient_WinRt();
    virtual ~HttpClient_WinRt();
    virtual IHttpRequest* CreateRequest() override;
    virtual void SendRequestAsync(IHttpRequest* request, IHttpResponseCallback* callback) override;
    virtual void CancelRequestAsync(std::string const& id) override;
    virtual void CancelAllRequests() override;
    HttpClient getHttpClient() { return m_httpClient; }

  protected:
    void erase(std::string const& id);
   
  protected:
    HttpClient                                                       m_httpClient;
    std::mutex                                                       m_requestsMutex;
    std::map<std::string, WinRtRequestWrapper*>                      m_requests;
    static unsigned                                                  s_nextRequestId;

    friend class WinRtRequestWrapper;
};

} MAT_NS_END

#endif // HAVE_MAT_DEFAULT_HTTP_CLIENT

#endif // HTTPCLIENT_WINRT_HPP

