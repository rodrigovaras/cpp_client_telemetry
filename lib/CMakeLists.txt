include_directories( . ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/public ${CMAKE_CURRENT_SOURCE_DIR}/include/public ${CMAKE_CURRENT_SOURCE_DIR}/include/mat ${CMAKE_CURRENT_SOURCE_DIR}/pal ${CMAKE_CURRENT_SOURCE_DIR}/utils ${CMAKE_CURRENT_SOURCE_DIR}/modules/exp ${CMAKE_CURRENT_SOURCE_DIR}/modules/dataviewer ${CMAKE_CURRENT_SOURCE_DIR}/modules/privacyguard ${CMAKE_CURRENT_SOURCE_DIR}/modules/liveeventinspector ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/Reachability ${CMAKE_CURRENT_SOURCE_DIR}/modules/cds /usr/local/include )

set(SRCS decorators/BaseDecorator.cpp
  packager/BondSplicer.cpp
  packager/Packager.cpp
  callbacks/DebugSource.cpp
  bond/BondSerializer.cpp
  filter/EventFilterCollection.cpp
  tpm/TransmitProfiles.cpp
  tpm/TransmissionPolicyManager.cpp
  tpm/DeviceStateHandler.cpp
  system/EventProperty.cpp
  system/TelemetrySystem.cpp
  system/EventProperties.cpp
  compression/HttpDeflateCompression.cpp
  api/AllowedLevelsCollection.cpp
  api/LogManager.cpp
  api/ContextFieldsProvider.cpp
  api/LogManagerImpl.cpp
  api/LogSessionData.cpp
  api/Logger.cpp
  api/LogManagerProvider.cpp
  api/CorrelationVector.cpp
  api/LogConfiguration.cpp
  api/AuthTokensController.cpp
  api/ILogConfiguration.cpp
  api/LogManagerFactory.cpp
  api/capi.cpp
  api/DataViewerCollection.cpp
  utils/FileUtils.cpp
  utils/Utils.cpp
  utils/StringUtils.cpp
  utils/ZlibUtils.cpp
  pal/InformationProviderImpl.cpp
  http/HttpClient_CAPI.cpp
  http/HttpClientManager.cpp
  http/HttpRequestEncoder.cpp
  http/HttpResponseDecoder.cpp
  http/HttpClientFactory.cpp
  stats/Statistics.cpp
  stats/MetaStats.cpp
  offline/StorageObserver.cpp
  offline/OfflineStorageFactory.cpp
  offline/MemoryStorage.cpp
  offline/OfflineStorage_SQLite.cpp
  offline/OfflineStorageHandler.cpp
  offline/LogSessionDataProvider.cpp
  backoff/IBackoff.cpp
  pal/PAL.cpp
  pal/TaskDispatcher_CAPI.cpp
  pal/WorkerThread.cpp
)

# Support for Azure Monitor / Application Insights
if(BUILD_AZMON)
  include(modules/azmon/CMakeLists.txt OPTIONAL)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/exp/)
  list(APPEND SRCS
    modules/exp/afd/afdclient/AFDClientUtils.cpp
    modules/exp/afd/afdclient/AFDClient.cpp
    modules/exp/afd/afdclient/AFDConfigCache.cpp
    modules/exp/ecs/ecsclient/ECSClient.cpp
    modules/exp/ecs/ecsclient/ECSClientUtils.cpp
    modules/exp/ecs/ecsclient/ECSConfigCache.cpp
    modules/exp/JsonHelper.cpp
    modules/exp/EXPCommonClient.cpp
    modules/exp/FifoFileStorage.cpp
  )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/dataviewer/)
  list(APPEND SRCS
    modules/dataviewer/DefaultDataViewer.cpp
    modules/dataviewer/OnDisableNotificationCollection.cpp
  )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/privacyguard/ AND BUILD_PRIVACYGUARD)
  list(APPEND SRCS
    modules/privacyguard/PrivacyGuard.cpp
    modules/privacyguard/RegisteredFileTypes.cpp
    modules/privacyguard/SummaryStatistics.cpp
  )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/liveeventinspector/ AND BUILD_LIVEEVENTINSPECTOR)
  list(APPEND SRCS
    modules/liveeventinspector/LiveEventInspector.cpp
    modules/liveeventinspector/LiveEventInspector.hpp
  )
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/cds/ AND BUILD_CDS)
  add_definitions(-DHAVE_MAT_CDS)
  list(APPEND SRCS
    modules/cds/CdsFactory.hpp
    modules/cds/CdsFactory.cpp
  )
endif()

find_package(unofficial-sqlite3 REQUIRED)
find_package(ZLIB REQUIRED)

if(PAL_IMPLEMENTATION STREQUAL "CPP11")
  if(APPLE)
    list(APPEND SRCS
      pal/posix/sysinfo_utils_apple.cpp
    )
    if(BUILD_IOS)
      list(APPEND SRCS
        pal/posix/sysinfo_utils_ios.mm
      )
    else()
      list(APPEND SRCS
        pal/posix/sysinfo_utils_mac.mm
      )
    endif()
  endif()
  list(APPEND SRCS
    pal/posix/DeviceInformationImpl.cpp
    pal/posix/SystemInformationImpl.cpp
    pal/posix/sysinfo_sources.cpp
  )

  if(APPLE)
    if(BUILD_APPLE_HTTP OR BUILD_IOS)
      list(APPEND SRCS
        http/HttpClient_Apple.mm
      )
    else()
      list(APPEND SRCS
        http/HttpClient_Curl.cpp
        http/HttpClient_Curl.hpp
      )
    endif()

    list(APPEND SRCS
        pal/posix/NetworkInformationImpl.mm
        # TODO: this unit below needs to be deprecated and removed
        ../third_party/Reachability/ODWReachability.m
    )
  else()
    list(APPEND SRCS
      http/HttpClient_Curl.cpp
      http/HttpClient_Curl.hpp
      pal/posix/NetworkInformationImpl.cpp
    )
  endif()
  if(APPLE AND BUILD_OBJC_WRAPPER)
	message("Include Wrappers")
	list(APPEND SRCS
	  ../wrappers/obj-c/ODWLogger.mm
	  ../wrappers/obj-c/ODWLogManager.mm
	  ../wrappers/obj-c/ODWEventProperties.mm
	  ../wrappers/obj-c/ODWLogConfiguration.mm
	  ../wrappers/obj-c/ODWSemanticContext.mm
	  )
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/dataviewer/)
  		list(APPEND SRCS
		  ../wrappers/obj-c/ODWDiagnosticDataViewer.mm
  		)
	endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/privacyguard/ AND BUILD_PRIVACYGUARD)
          list(APPEND SRCS
          ../wrappers/obj-c/ODWCommonDataContext.mm
          ../wrappers/obj-c/ODWPrivacyGuard.mm
          ../wrappers/obj-c/ODWPrivacyGuardInitConfig.mm
          )
    endif()
  endif()
elseif(PAL_IMPLEMENTATION STREQUAL "WIN32")
  # Win32 Desktop for now.
  # TODO: define a separate PAL for Win10 cmake build
  #include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../zlib ${CMAKE_CURRENT_SOURCE_DIR}/../sqlite)
  add_definitions(-D_UNICODE -DUNICODE -DZLIB_WINAPI -DWIN32 -DMATSDK_PLATFORM_WINDOWS=1 -D_UTC_SDK -D_CRT_SECURE_NO_WARNINGS -DUSE_BOND -D_WINDOWS -D_USRDLL -DWINVER=_WIN32_WINNT_WIN7)
  add_definitions(/wd4005 /wd4503)
  
  remove_definitions(-D_MBCS)
  list(APPEND SRCS
    http/HttpClient_WinInet.cpp
    http/HttpClient_WinInet.hpp
    pal/desktop/WindowsDesktopDeviceInformationImpl.cpp
    pal/desktop/WindowsDesktopNetworkInformationImpl.cpp
    pal/desktop/WindowsDesktopSystemInformationImpl.cpp
    pal/desktop/WindowsEnvironmentInfo.hpp
    pal/desktop/NetworkDetector.cpp
    utils/StringConversion.cpp
  )

  # UTC module
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/utc)
    list(APPEND SRCS
      modules/utc/desktop/UtcHelpers.cpp
      modules/utc/UtcTelemetrySystem.cpp
    )
  endif()

elseif(PAL_IMPLEMENTATION STREQUAL "GAMECORE")
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../zlib ${CMAKE_CURRENT_SOURCE_DIR}/../sqlite ${CMAKE_CURRENT_SOURCE_DIR}/pal/gamecore)
  add_definitions(-D_UNICODE -DUNICODE -DZLIB_WINAPI -DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -D_USRDLL -DWINVER=_WIN32_WINNT_WIN7 -DWINAPI_FAMILY=WINAPI_FAMILY_GAMES -DSQLITE_DEFAULT_AUTOVACUUM=1 -DSQLITE_NO_SYNC)
  add_definitions(/wd4005 /wd4503 /wd4267)
  remove_definitions(-D_MBCS)
  list(APPEND SRCS
    http/HttpClient_WinHttp.cpp
    http/HttpClient_WinHttp.hpp
    pal/desktop/WindowsDesktopDeviceInformationImpl.cpp
    pal/desktop/WindowsDesktopNetworkInformationImpl.cpp
    pal/desktop/WindowsDesktopSystemInformationImpl.cpp
    utils/StringConversion.cpp
  )
elseif(PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT")
  include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/../zlib ${CMAKE_CURRENT_SOURCE_DIR}/../sqlite ${CMAKE_CURRENT_SOURCE_DIR}/pal/universal ${CMAKE_CURRENT_SOURCE_DIR}/shared "${CMAKE_CURRENT_SOURCE_DIR}/../Solutions/win10-lib/Generated Files/")
  add_definitions(-D_UNICODE -DUNICODE -DZLIB_WINAPI -DWIN32 -D_WINRT_DLL -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -D_WINRT -D_CRT_SECURE_NO_WARNINGS)
  add_definitions(/wd4005)
  remove_definitions(-D_MBCS)
  list(APPEND SRCS
    http/HttpClient_WinRt.cpp
    http/HttpClient_WinRt.hpp
    pal/universal/pch.h
    pal/universal/WindowsRTHelpers.cpp
    pal/universal/WindowsRuntimeDeviceInformationImpl.cpp
    pal/universal/WindowsRuntimeNetworkInformationImpl.cpp
    pal/universal/WindowsRuntimeSystemInformationImpl.cpp
    utils/StringConversion.cpp
  )

  list(APPEND IMPORT_PROJECTS
    ${CPPWINRT_NUGET_PATH}/build/native/Microsoft.Windows.CppWinRT.props
    ${CPPWINRT_NUGET_PATH}/build/native/Microsoft.Windows.CppWinRT.targets
  )

  # UTC module
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/utc)
    list(APPEND SRCS
      modules/utc/universal/UtcHelpers.cpp
      modules/utc/UtcTelemetrySystem.cpp
    )
  endif()

else()
  message(FATAL_ERROR "No platform abstraction library configured. Set PAL_IMPLEMENTATION.")
endif()

# Filtering module
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/filter)
  list(APPEND SRCS
    modules/filter/CompliantByDefaultEventFilterModule.cpp
    modules/filter/CompliantByDefaultFilterApi.cpp
    modules/filter/LevelCheckingEventFilter.cpp
  )
endif()

create_source_files_groups_per_folder(${SRCS})

# MinGW does not require pthread
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
   target_compile_options(mat PUBLIC "-pthread")
  endif()
endif()

if(BUILD_SHARED_LIBS STREQUAL "ON")
  message("-- Building shared SDK library")
  
  # include(FindCURL)
  # find_package(CURL REQUIRED)
  # set(CMAKE_REQUIRED_LIBRARIES "${CURL_LIBRARIES}")

  # find_package(sqlite3 REQUIRED)

  add_library(mat SHARED ${SRCS})

  if((PAL_IMPLEMENTATION STREQUAL "WIN32") OR (PAL_IMPLEMENTATION STREQUAL "GAMECORE") OR (PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT"))
    target_compile_options(mat PUBLIC /std:c++20)
  endif()

  if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    add_library(mat_slimcore SHARED ${SRCS})
    target_compile_definitions(mat_slimcore PUBLIC -D_PPLTASK_ASYNC_LOGGING=0)
    target_compile_options(mat_slimcore PUBLIC /std:c++17)
  endif()

  if(PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT")
    set_target_properties(mat PROPERTIES VS_PROJECT_IMPORT "${IMPORT_PROJECTS}")
    
    if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
      set_target_properties(mat_slimcore PROPERTIES VS_PROJECT_IMPORT "${IMPORT_PROJECTS}")
    endif()

  endif()

  if(PAL_IMPLEMENTATION STREQUAL "WIN32")
    target_link_libraries(mat PUBLIC wininet.lib Crypt32.lib)
    if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
      target_link_libraries(mat_slimcore PUBLIC wininet.lib Crypt32.lib)
    endif()
  endif()


  # Add flags for obtaining system UUID via IOKit
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_SHARED_LINKER_FLAGS
      "-framework CoreFoundation -framework IOKit -framework Foundation"
    )
  endif()

  #
  # TODO: allow adding "${Tcmalloc_LIBRARIES}" to target_link_libraries for memory leak debugging
  #
  # if (BUILD_STATIC_SQLITE STREQUAL "ON")
  #   # Build dynamic library, but prefer statically linking sqlite3 and zlib
  #   add_library(sqlite3 STATIC IMPORTED GLOBAL)
  #   add_library(z STATIC IMPORTED GLOBAL)
  #   target_link_libraries(mat PRIVATE sqlite3 PUBLIC z ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" "${CMAKE_REQUIRED_LIBRARIES}")

  #   if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
  #     target_link_libraries(mat_slimcore PRIVATE sqlite3 PUBLIC z ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" "${CMAKE_REQUIRED_LIBRARIES}")
  #   endif()
  # else()
  #   # Prefer shared libraries for sqlite3 and zlib
  #   add_library(sqlite3 SHARED IMPORTED GLOBAL)
  #   add_library(z SHARED IMPORTED GLOBAL)
  #   target_link_libraries(mat PUBLIC sqlite3 PUBLIC z ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" "${CMAKE_REQUIRED_LIBRARIES}")

  #   if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
  #     target_link_libraries(mat_slimcore PUBLIC sqlite3 PUBLIC z ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" "${CMAKE_REQUIRED_LIBRARIES}")
  #   endif()
  # endif()

  # target_link_libraries(mat PUBLIC libsqlite3 libcurl.a libz.a libssl.a libcrypto.a "${SQLITE_LIBRARY}" "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
  install(TARGETS mat EXPORT mat LIBRARY DESTINATION ${INSTALL_LIB_DIR})
  if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    install(TARGETS mat_slimcore EXPORT mat_slimcore LIBRARY DESTINATION ${INSTALL_LIB_DIR})
  endif()
else()
  message("-- Building static SDK library")
  add_library(mat STATIC ${SRCS})
  if((PAL_IMPLEMENTATION STREQUAL "WIN32") OR (PAL_IMPLEMENTATION STREQUAL "GAMECORE") OR (PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT"))
    target_compile_options(mat PUBLIC /std:c++20)
  endif()

  if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    add_library(mat_slimcore STATIC ${SRCS})
    target_compile_definitions(mat_slimcore PUBLIC -D_PPLTASK_ASYNC_LOGGING=0)
    target_compile_options(mat_slimcore PUBLIC /std:c++17)
  endif()

  if(PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT")
    set_target_properties(mat PROPERTIES VS_PROJECT_IMPORT "${IMPORT_PROJECTS}")
    if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
      set_target_properties(mat_slimcore PROPERTIES VS_PROJECT_IMPORT "${IMPORT_PROJECTS}")
    endif()
  endif()

  # add_library(sqlite3 STATIC IMPORTED GLOBAL)
  # add_library(z STATIC IMPORTED GLOBAL)

  if((PAL_IMPLEMENTATION STREQUAL "WIN32") OR (PAL_IMPLEMENTATION STREQUAL "GAMECORE") OR (PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT"))
    #target_link_libraries(mat PRIVATE sqlite3.lib PUBLIC zlibd.lib ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )

    # if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    #   target_link_libraries(mat_slimcore PRIVATE sqlite3.lib PUBLIC zlibd.lib ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
    # endif()
    if(PAL_IMPLEMENTATION STREQUAL "WIN10_WINRT")
      target_link_libraries(mat PUBLIC WindowsApp.lib)
      if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
        target_link_libraries(mat_slimcore PUBLIC WindowsApp.lib)
      endif()

    endif()
  else()
    #
    # TODO: allow adding "${Tcmalloc_LIBRARIES}" to target_link_libraries for memory leak debugging
    #
    #target_link_libraries(mat PRIVATE libsqlite3.a PUBLIC libz.a ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
  endif()
  
  install(TARGETS mat EXPORT mat ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    install(TARGETS mat_slimcore EXPORT mat_slimcore ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  endif()
endif()

if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
  target_link_libraries(mat_slimcore PRIVATE unofficial::sqlite3::sqlite3 PUBLIC ZLIB::ZLIB ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
endif()

target_link_libraries(mat PRIVATE unofficial::sqlite3::sqlite3 PUBLIC ZLIB::ZLIB ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )

message("-- Library will be installed to ${INSTALL_LIB_DIR}")

#if(PAL_IMPLEMENTATION STREQUAL "CPP11")
#  #target_link_libraries(mat PUBLIC libcurl.a libz.a libssl.a libcrypto.a "${SQLITE_LIBRARY}" "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
#  #target_link_libraries(mat PUBLIC libsqlite3.a libz.a ${LIBS} "${CMAKE_THREAD_LIBS_INIT}" "${CMAKE_DL_LIBS}" )
#endif()

set(TARGETS_TO_INSTALL mat)
if(ENABLE_SLIMCORE_BUILD STREQUAL "ON")
    list(APPEND TARGETS_TO_INSTALL mat_slimcore)
endif()

install(TARGETS ${TARGETS_TO_INSTALL}
  EXPORT cpp-client-telemetry-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)

install(EXPORT cpp-client-telemetry-targets
  NAMESPACE cpp-client-telemetry::
  DESTINATION share/cpp-client-telemetry)

install(
    FILES cpp-client-telemetry-config.cmake
    DESTINATION share/cpp-client-telemetry)
