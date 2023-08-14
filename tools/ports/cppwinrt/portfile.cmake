
# Define hardcoded versions for cppwinrt & winsdk contracts
set(cppwinrt_version 2.0.220531.1)
set(winsdk_version 10.0.18362.2005)

vcpkg_download_distfile(ARCHIVE
    URLS "https://www.nuget.org/api/v2/package/Microsoft.Windows.CppWinRT/${cppwinrt_version}"
    FILENAME "cppwinrt.${cppwinrt_version}.zip"
    SHA512 159084ffce60f67a1b558799983f702b3e122a41f9012de21bee69d2761f6f3a53c3c807a99c0ba7cc276ccf25ce19ead89174eca4dbb23238dff1780bd49eea
)

vcpkg_extract_source_archive(
    src
    ARCHIVE "${ARCHIVE}"
    NO_REMOVE_ONE_LEVEL
)

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(CPPWINRT_ARCH win32)
else()
    set(CPPWINRT_ARCH ${VCPKG_TARGET_ARCHITECTURE})
endif()

set(CPPWINRT_TOOL "${src}/bin/cppwinrt.exe")

vcpkg_download_distfile(ARCHIVE_SDK
    URLS "https://www.nuget.org/api/v2/package/Microsoft.Windows.SDK.Contracts/${winsdk_version}"
    FILENAME "winsdk.${winsdk_version}.zip"
    SHA512 7fb524be27906e1b7e713830a5260068b2a5f9613a65393a9961da04c59506b1b84600384b1289c07f2415bf83e601be07242a3cb936243099d0fb4b65100f40
)

vcpkg_extract_source_archive(
    winsdk
    ARCHIVE "${ARCHIVE_SDK}"
    NO_REMOVE_ONE_LEVEL
)

file(GLOB winmds "${winsdk}/*/*/*.winmd")

#--- Create response file
set(args "")
foreach(winmd IN LISTS winmds)
    string(APPEND args "-input \"${winmd}\"\n")
endforeach()

file(REMOVE_RECURSE "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}")
file(MAKE_DIRECTORY "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}")
file(WRITE "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}/cppwinrt.rsp" "${args}")

#--- Generate headers
message(STATUS "Generating headers for Windows SDK $ENV{WindowsSDKVersion}")
vcpkg_execute_required_process(
    COMMAND "${CPPWINRT_TOOL}"
        "@${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}/cppwinrt.rsp"
        -output "${CURRENT_PACKAGES_DIR}/include"
        -verbose
    WORKING_DIRECTORY "${CURRENT_PACKAGES_DIR}"
    LOGNAME "cppwinrt-generate-${TARGET_TRIPLET}"
)

set(CPPWINRT_LIB "${src}/build/native/lib/${CPPWINRT_ARCH}/cppwinrt_fast_forwarder.lib")
file(COPY "${CPPWINRT_LIB}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
if(NOT VCPKG_BUILD_TYPE)
    file(COPY "${CPPWINRT_LIB}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()
file(COPY
    "${CPPWINRT_TOOL}"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/cppwinrt")
file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/cppwinrt-config.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/cppwinrt")

configure_file("${src}/LICENSE" "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright" COPYONLY)
