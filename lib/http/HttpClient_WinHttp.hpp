// Copyright (c) Microsoft Corporation. All rights reserved.
#ifndef HTTPCLIENT_WINHTTP_HPP
#define HTTPCLIENT_WINHTTP_HPP

#ifdef HAVE_MAT_DEFAULT_HTTP_CLIENT

#include "IHttpClient.hpp"
#include "pal/PAL.hpp"

#include "ILogManager.hpp"

namespace MAT_NS_BEGIN {

#ifndef _WINHTTPX_
typedef void* HINTERNET;
#endif

class WinHttpRequestWrapper;

class HttpClient_WinHttp : public IHttpClient {
  public:
    // Common IHttpClient methods
    HttpClient_WinHttp();
    virtual ~HttpClient_WinHttp();
    virtual IHttpRequest* CreateRequest() final;
    virtual void SendRequestAsync(IHttpRequest* request, IHttpResponseCallback* callback) final;
    virtual void CancelRequestAsync(std::string const& id) final;
    virtual void CancelAllRequests() final;
  
  protected:
    void erase(std::string const& id);

  protected:
    HINTERNET                                                        m_hInternet;
    std::recursive_mutex                                             m_requestsMutex;
    std::map<std::string, WinHttpRequestWrapper*>                    m_requests;
    static unsigned                                                  s_nextRequestId;
    friend class WinHttpRequestWrapper;
};

} MAT_NS_END

#endif  // HAVE_MAT_DEFAULT_HTTP_CLIENT

#endif  // HTTPCLIENT_WINHTTP_HPP