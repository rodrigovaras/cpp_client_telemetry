//
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
#include "mat/config.h"

#ifdef HAVE_MAT_DEFAULT_HTTP_CLIENT

#include "HttpClientFactory.hpp"
#include "pal/PAL.hpp"

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(MATSDK_PAL_WIN32)
  #ifdef _WINRT_DLL
    #include "http/HttpClient_WinRt.hpp"
  #elif defined(HAVE_MAT_WININET_HTTP_CLIENT)
    #include "http/HttpClient_WinInet.hpp"
  #else
    #include "http/HttpClient_WinHttp.hpp"
  #endif
#elif defined(MATSDK_PAL_CPP11)
  #if TARGET_OS_IPHONE || (defined(__APPLE__) && defined(APPLE_HTTP))
    #include "http/HttpClient_Apple.hpp"
  #elif defined(HAVE_MAT_CURL_HTTP_CLIENT) || !defined(ANDROID)
    #include "http/HttpClient_Curl.hpp"
  #elif defined(ANDROID)
    #include "http/HttpClient_Android.hpp"
  #endif
#else
  #error The library cannot work without an HTTP client implementation.
#endif

namespace MAT_NS_BEGIN {

    MATSDK_LOG_INST_COMPONENT_CLASS(HttpClientFactory, "EventsSDK.HttpClientFactory", "Events telemetry client - HttpClientFactory class");

#if defined(MATSDK_PAL_WIN32)
#ifdef _WINRT_DLL
    /* Win 10 HTTP client */
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_WinRt");
        return std::make_shared<HttpClient_WinRt>();
    }
#elif defined(HAVE_MAT_WININET_HTTP_CLIENT)
    /* Win32 WinInet HTTP client */
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_WinInet");
        return std::make_shared<HttpClient_WinInet>();
    }
#else
    /* Xplat WinHTTP HTTP client */
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_WinHttp");
        return std::make_shared<HttpClient_WinHttp>();
    }
#endif
#elif defined(HAVE_MAT_CURL_HTTP_CLIENT)
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_Curl");
        return std::make_shared<HttpClient_Curl>();
    }
#elif defined(MATSDK_PAL_CPP11)
#if TARGET_OS_IPHONE || (defined(__APPLE__) && defined(APPLE_HTTP))
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_Apple");
        return std::make_shared<HttpClient_Apple>();
    }
#elif defined(ANDROID)
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_Android");
        return HttpClient_Android::GetClientInstance();
    }
#else
    std::shared_ptr<IHttpClient> HttpClientFactory::Create() {
        LOG_TRACE("Creating HttpClient_Curl");
        return std::make_shared<HttpClient_Curl>();
    }
#endif
#else
#error The library cannot work without an HTTP client implementation.
#endif

} MAT_NS_END


#endif

