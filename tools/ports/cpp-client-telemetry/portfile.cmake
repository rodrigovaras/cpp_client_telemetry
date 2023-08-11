set(VERSION 3.5.189)

# vcpkg_from_git(
#     OUT_SOURCE_PATH SOURCE_PATH
#     URL https://github.com/microsoft/cpp_client_telemetry
#     REF 6224f89113cd3da1b6127b4b1dfbb06ed00962f5
# )

# if(DEFINED ENV{CPP_TELEMETRY_PRIVATE_MODULE_TOKEN})
#     vcpkg_from_github(
#         OUT_SOURCE_PATH MODULE_SOURCE_PATH
#         REPO microsoft/cpp_client_telemetry_modules
#         REF d62105a9e95c2390405173eb97e9f7fab05ba696
#         SHA512 361b769f3b4e2ddabbc7d3ab15dcc18704c382cc1926e7b42ebb0487cd1e7020236165041217aab4f1a2f0e492af502fec74c1a2411b78a18649f83c0b658b3a
#         AUTHORIZATION_TOKEN $ENV{CPP_TELEMETRY_PRIVATE_MODULE_TOKEN}
#         PATCHES
#             fixed_modules.patch
#     )
# else()
#     vcpkg_from_git(
#         OUT_SOURCE_PATH MODULE_SOURCE_PATH
#         URL  https://github.com/microsoft/cpp_client_telemetry_modules
#         REF d62105a9e95c2390405173eb97e9f7fab05ba696
#         PATCHES
#             fixed_modules.patch
#     )
# endif()

# file (COPY ${MODULE_SOURCE_PATH}/ DESTINATION ${SOURCE_PATH}/lib/modules/)

get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
message("Using local source snapshot from ${SOURCE_PATH}")

if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    list(APPEND PLATFORM_OPTIONS "-DBUILD_SHARED_LIBS=ON")
else()
    list(APPEND PLATFORM_OPTIONS "-DBUILD_SHARED_LIBS=OFF")
endif()

list(APPEND PLATFORM_OPTIONS "-DBUILD_UNIT_TESTS=NO" "-DBUILD_FUNC_TESTS=NO")

if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL Android)
    list(APPEND PLATFORM_OPTIONS "-DANDROID_STL=c++_shared" "-DUSE_ROOM=0" "-DUSE_CURL=1" "-DPAL_IMPLEMENTATION=CPP11")

    vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}/lib/android_build/maesdk/src/main/cpp
        OPTIONS
            ${PLATFORM_OPTIONS}
            -DCMAKE_PACKAGE_TYPE=tgz
            -DSKIP_INSTALL_PROGRAMS=ON
            -DSKIP_INSTALL_EXECUTABLES=ON
            -DSKIP_INSTALL_FILES=ON
        OPTIONS_DEBUG
            -DSKIP_INSTALL_HEADERS=ON
    )
elseif((VCPKG_CMAKE_SYSTEM_NAME STREQUAL Darwin) OR (VCPKG_CMAKE_SYSTEM_NAME STREQUAL Linux))

    if((VCPKG_TARGET_ARCHITECTURE STREQUAL x86) OR (VCPKG_TARGET_ARCHITECTURE STREQUAL x64))
        if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL Darwin)
            list(APPEND PLATFORM_OPTIONS "-DMAC_ARCH=x86_64")
        elseif(VCPKG_CMAKE_SYSTEM_NAME STREQUAL Linux)
            list(APPEND PLATFORM_OPTIONS "-DPAL_IMPLEMENTATION=CPP11")
        endif()
    else()
        message(FATAL_ERROR "Unsupported ${VCPKG_CMAKE_SYSTEM_NAME} architecture!")
    endif()

    vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
            ${PLATFORM_OPTIONS}
            -DCMAKE_PACKAGE_TYPE=tgz
            -DBUILD_UNIT_TESTS=OFF
            -DBUILD_FUNC_TESTS=OFF
    )
else()
    if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
        list(APPEND PLATFORM_OPTIONS "-DPAL_IMPLEMENTATION=WIN10_WINRT")
    elseif(NOT DEFINED VCPKG_CMAKE_SYSTEM_NAME)
        string(FIND "${VCPKG_CXX_FLAGS}" "/DWINAPI_FAMILY=WINAPI_FAMILY_GAMES" IS_GAMECORE_SYSTEM_NAME)

        if(${IS_GAMECORE_SYSTEM_NAME} STREQUAL -1)
            # VCPKG_CMAKE_SYSTEM_NAME is Windows
            list(APPEND PLATFORM_OPTIONS "-DPAL_IMPLEMENTATION=WIN32")
        else()
            list(APPEND PLATFORM_OPTIONS "-DPAL_IMPLEMENTATION=GAMECORE")
        endif()
    endif()

    # ENABLE_SLIMCORE_BUILD is an option we introduced to generate a custom version of telemetry
    # It will help to produce mat_slimcore.lib with _PPLTASK_ASYNC_LOGGING=0
    # See uwp_disable_ppltask_async_logging.patch
    set(ENABLE_SLIMCORE_BUILD 1)
    list(APPEND PLATFORM_OPTIONS "-DENABLE_SLIMCORE_BUILD=ON")

    vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
            ${PLATFORM_OPTIONS}
            -DCMAKE_PACKAGE_TYPE=tgz
            -DBUILD_UNIT_TESTS=OFF
            -DBUILD_FUNC_TESTS=OFF
            -DCMAKE_SYSTEM_VERSION=10.0.22000.0 #Minimum Win 10 SDK version to ensure successful build in the pipeline
    )
endif()

vcpkg_cmake_build()

vcpkg_cmake_install()

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")