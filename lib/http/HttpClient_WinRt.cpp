//
// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: Apache-2.0
//
#include "mat/config.h"

#ifdef HAVE_MAT_DEFAULT_HTTP_CLIENT

#include "pal/PAL.hpp"

#include "http/HttpClient_WinRt.hpp"
#include "utils/StringUtils.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

#include <pplcancellation_token.h>
#include <ppltasks.h>
#include <pplawait.h>

#include <winrt/Windows.Storage.Streams.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage::Streams;
using namespace concurrency;


namespace MAT_NS_BEGIN {

    class WinRtRequestWrapper
    {
    protected:
        HttpClient_WinRt&      m_parent;
        SimpleHttpRequest*     m_request;
        const std::string      m_id;
        IHttpResponseCallback* m_appCallback;
        HttpRequestMessage    m_httpRequestMessage;
        HttpResponseMessage   m_httpResponseMessage;
        concurrency::cancellation_token_source m_cancellationTokenSource;

    public:
        WinRtRequestWrapper(HttpClient_WinRt& parent, SimpleHttpRequest* request)
            : m_parent(parent),
            m_request(request),
            m_id(request->GetId()),
            m_appCallback(nullptr),
            m_httpRequestMessage(nullptr)
        {
            LOG_TRACE("%p WinRtRequestWrapper()", this);
        }

        WinRtRequestWrapper(WinRtRequestWrapper const&) = delete;

        WinRtRequestWrapper& operator=(WinRtRequestWrapper const&) = delete;

        ~WinRtRequestWrapper()
        {
            LOG_TRACE("%p ~WinRtRequestWrapper()", this);
        }


        void cancel()
        {
            m_cancellationTokenSource.cancel();
            //m_httpRequestMessage, don't need to delete, ref object framework will delete it
        }

        void send(IHttpResponseCallback* callback)
        {
            m_appCallback = callback;

            {
                std::lock_guard<std::mutex> lock(m_parent.m_requestsMutex);
                m_parent.m_requests[m_id] = this;
            }

            try
            {
                // Convert std::string to Uri
                Uri uri(to_platform_string(m_request->m_url));
                // Create new request message
                if (m_request->m_method.compare("GET") == 0)
                {
                    m_httpRequestMessage = HttpRequestMessage(HttpMethod::Get(), uri);
                }
                else
                {
                    m_httpRequestMessage = HttpRequestMessage(HttpMethod::Post(), uri);
                }

                // Initialize the in-memory stream where data will be stored.
                winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;

                DataWriter dataWriter{stream};
                dataWriter.WriteBytes(m_request->m_body);
                IBuffer ibuffer = dataWriter.DetachBuffer();
                HttpBufferContent httpBufferContent(ibuffer);

                m_httpRequestMessage.Content(httpBufferContent);  // ref new HttpBufferContent(ibuffer);

                // Populate headers based on user-supplied headers
                for (auto &kv : m_request->m_headers)
                {
                    winrt::hstring key = to_platform_string(kv.first);
                    winrt::hstring value = to_platform_string(kv.second);

                    if (kv.first.compare("Expect") == 0)
                    {
                        m_httpRequestMessage.Headers().Expect().TryParseAdd(value);
                        continue;
                    }
                    if (kv.first.compare("Content-Encoding") == 0)
                    {
                        m_httpRequestMessage.Content().Headers().ContentEncoding().Append(HttpContentCodingHeaderValue(value));
                        continue;
                    }
                    if (kv.first.compare("Content-Type") == 0)
                    {
                        m_httpRequestMessage.Content().Headers().ContentType(HttpMediaTypeHeaderValue(value));
                        continue;
                    }

                    m_httpRequestMessage.Headers().TryAppendWithoutValidation(key, value);
                }
            }
            catch (winrt::hresult_error)
            {
                HttpResponseMessage failed(HttpStatusCode::BadRequest);
                onRequestComplete(failed);
                return;
            }
            catch (...)
            {
                HttpResponseMessage failed(HttpStatusCode::BadRequest);
                onRequestComplete(failed);
                return;
            }

            SendHttpAsyncRequest(m_httpRequestMessage);
        }

        void SendHttpAsyncRequest(HttpRequestMessage req)
        {
            IAsyncOperationWithProgress<HttpResponseMessage, HttpProgress> operation = m_parent.getHttpClient().SendRequestAsync(req, HttpCompletionOption::ResponseContentRead);
            m_cancellationTokenSource = cancellation_token_source();
            auto cancellationToken = m_cancellationTokenSource.get_token();

            operation.Progress([this, cancellationToken](IAsyncOperationWithProgress<HttpResponseMessage, HttpProgress> const& sender, HttpProgress const&)
                               {
                                   if (cancellationToken.is_canceled())
                                   {
                                       sender.Cancel();
                                   }
                               });

            operation.Completed([this](IAsyncOperationWithProgress<HttpResponseMessage, HttpProgress> const& sender, AsyncStatus const)
                                {
                                    try
                                    {
                                        // Check if any previous task threw an exception.
                                        m_httpResponseMessage = sender.GetResults();
                                        // if (m_httpResponseMessage.StatusCode() != HttpStatusCode::None)
                                        {
                                            onRequestComplete(m_httpResponseMessage);
                                        }
                                    }
                                    catch (winrt::hresult_error const& ex)
                                    {
                                        HttpResponseMessage failed;
                                        if (ex.code().value == 0x80072ee7)
                                        {
                                            failed = HttpResponseMessage(HttpStatusCode::NotFound);
                                        }
                                        else
                                        {
                                            failed = HttpResponseMessage(HttpStatusCode::BadRequest);
                                        }

                                        onRequestComplete(failed);
                                    }
                                    catch (...)
                                    {
                                        HttpResponseMessage failed(HttpStatusCode::BadRequest);
                                        onRequestComplete(failed);
                                    }
                                });
        }

        void onRequestComplete(HttpResponseMessage httpResponse)
        {
            std::unique_ptr<SimpleHttpResponse> response(new SimpleHttpResponse(m_id));
            response->m_statusCode = static_cast<unsigned int>(httpResponse.StatusCode());
            if (httpResponse.IsSuccessStatusCode())
            {
                response->m_result = HttpResult_OK;
                IMapView<winrt::hstring, winrt::hstring> mapView = httpResponse.Headers().GetView();

                auto iterator = mapView.First();
                unsigned int  index = 0;
                while (index < mapView.Size())
                {
                    winrt::hstring Key = iterator.Current().Key();
                    winrt::hstring Value = iterator.Current().Value();

                    response->m_headers.add(from_platform_string(Key), from_platform_string(Value));
                    iterator.MoveNext();
                    index++;
                }

                auto operation = m_httpResponseMessage.Content().ReadAsBufferAsync();
                operation.Completed([this, &response](winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Windows::Storage::Streams::IBuffer, uint64_t> const& sender, AsyncStatus const)
                                    {
                                        IMapView<winrt::hstring, winrt::hstring> contentHeadersView = m_httpResponseMessage.Content().Headers().GetView();

                                        auto contentHeadersiterator = contentHeadersView.First();
                                        unsigned int contentHeadersIndex = 0;
                                        while (contentHeadersIndex < contentHeadersView.Size())
                                        {
                                            winrt::hstring Key = contentHeadersiterator.Current().Key();
                                            winrt::hstring Value = contentHeadersiterator.Current().Value();

                                            response->m_headers.add(from_platform_string(Key), from_platform_string(Value));
                                            contentHeadersiterator.MoveNext();
                                            contentHeadersIndex++;
                                        }

                                        auto buffer = sender.GetResults();
                                        size_t length = buffer.Length();

                                        if (length > 0)
                                        {
                                            response->m_body.reserve(length);
                                            response->m_body.resize(length);
                                            auto dataReader = DataReader::FromBuffer(buffer);
                                            dataReader.ReadBytes(response->m_body); /*(Platform::ArrayReference<unsigned char>(reinterpret_cast<unsigned char*>(response->m_body.data()), (DWORD)length)));*/
                                            dataReader.DetachBuffer();
                                        }
                                    });
            }
            else
            {
                if (httpResponse.StatusCode() == HttpStatusCode::BadRequest)
                {
                    response->m_result = HttpResult_LocalFailure;
                }
                else
                {
                    response->m_result = HttpResult_NetworkFailure;
                }
            }

            // 'response' gets released in EventsUploadContext.clear()
            m_appCallback->OnHttpResponse(response.release());
            m_parent.erase(m_id);

        }
    };

    //---

    unsigned HttpClient_WinRt::s_nextRequestId = 0;

    HttpClient_WinRt::HttpClient_WinRt()
    {
        m_httpClient = HttpClient();
    }

    HttpClient_WinRt::~HttpClient_WinRt()
    {
        bool done;

        {
            std::lock_guard<std::mutex> lock(m_requestsMutex);
            for (auto const& item : m_requests)
            {
                WinRtRequestWrapper* temp = item.second;
                temp->cancel();
            }
            done = m_requests.empty();
        }

        while (!done) {
            PAL::sleep(100);
            {
                std::lock_guard<std::mutex> lock(m_requestsMutex);
                done = m_requests.empty();
            }
        }
    }


    void HttpClient_WinRt::erase(std::string const& id)
    {
        std::lock_guard<std::mutex> lock(m_requestsMutex);
        auto it = m_requests.find(id);
        if (it != m_requests.end())
        {
            auto req = it->second;
            m_requests.erase(it);
            delete req;
        }
    }

    IHttpRequest* HttpClient_WinRt::CreateRequest()
    {
        std::string id = "WI-" + toString(::InterlockedIncrement(&s_nextRequestId));
        return new SimpleHttpRequest(id);
    }

    void HttpClient_WinRt::SendRequestAsync(IHttpRequest* request, IHttpResponseCallback* callback)
    {
        // Note: 'request' is never owned by IHttpClient and gets deleted in EventsUploadContext.clear()
        if (request==nullptr)
        {
            LOG_ERROR("request is null!");
            return;
        }
        SimpleHttpRequest* req = static_cast<SimpleHttpRequest*>(request);
        WinRtRequestWrapper* wrapper = new WinRtRequestWrapper(*this, req);
        wrapper->send(callback);
    }

    void HttpClient_WinRt::CancelRequestAsync(std::string const& id)
    {
        WinRtRequestWrapper* request = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_requestsMutex);
            auto it = m_requests.find(id);
            if (it != m_requests.end())
            {
                request = it->second;
                if (request) {
                    request->cancel();
                }
            }
        }
    }

    void HttpClient_WinRt::CancelAllRequests()
    {
        // vector of all request IDs
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lock(m_requestsMutex);
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

#endif //HAVE_MAT_DEFAULT_HTTP_CLIENT
