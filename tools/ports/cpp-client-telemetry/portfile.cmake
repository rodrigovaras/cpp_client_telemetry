set(VERSION 3.5.189)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL https://github.com/rodrigovaras/cpp_client_telemetry.git
    REF a5b4f5411d2342463a1877f2ab08ed351901669e
)

# get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
# message("Using local source snapshot from ${SOURCE_PATH}")

if(VCPKG_LIBRARY_LINKAGE STREQUAL dynamic)
    list(APPEND PLATFORM_OPTIONS "-DBUILD_SHARED_LIBS=ON")
else()
    list(APPEND PLATFORM_OPTIONS "-DBUILD_SHARED_LIBS=OFF")
endif()

list(APPEND PLATFORM_OPTIONS "-DBUILD_UNIT_TESTS=NO" "-DBUILD_FUNC_TESTS=NO")

if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL Android)
    list(APPEND PLATFORM_OPTIONS "-DANDROID_STL=c++_shared" "-DUSE_ROOM=0" "-DUSE_CURL=1" "-DPAL_IMPLEMENTATION=CPP11")

    vcpkg_cmake_configure(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
            ${PLATFORM_OPTIONS}
            -DCMAKE_PACKAGE_TYPE=tgz
            -DSKIP_INSTALL_PROGRAMS=ON
            -DSKIP_INSTALL_EXECUTABLES=ON
            -DSKIP_INSTALL_FILES=ON
            -DBUILD_UNIT_TESTS=OFF
            -DBUILD_FUNC_TESTS=OFF
            -DANDROID=1
        OPTIONS_DEBUG
            -DSKIP_INSTALL_HEADERS=ON
    )
elseif((VCPKG_CMAKE_SYSTEM_NAME STREQUAL Darwin) OR (VCPKG_CMAKE_SYSTEM_NAME STREQUAL Linux))

message("VCPKG_TARGET_ARCHITECTURE:${VCPKG_TARGET_ARCHITECTURE}")

    if((VCPKG_TARGET_ARCHITECTURE STREQUAL x86) OR (VCPKG_TARGET_ARCHITECTURE STREQUAL x64) OR (VCPKG_TARGET_ARCHITECTURE STREQUAL arm64))
        if(VCPKG_CMAKE_SYSTEM_NAME STREQUAL Darwin)
            if (VCPKG_TARGET_ARCHITECTURE STREQUAL arm64)
                list(APPEND PLATFORM_OPTIONS "-DMAC_ARCH=arm64")
            else()
                list(APPEND PLATFORM_OPTIONS "-DMAC_ARCH=x86_64")
            endif()
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