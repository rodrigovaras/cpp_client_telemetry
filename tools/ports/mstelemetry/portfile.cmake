# vcpkg_from_github(
#     OUT_SOURCE_PATH SOURCE_PATH
#     REPO microsoft/cpp_client_telemetry
#     REF 2f0e0e69e6b3739d7c450ac3d38816aee45ac3c2
#     SHA512 54cbe4270963944edf3d75b845047add2b004e0d95b20b75a4c4790c2a12a41bf19cc4f55aaeaec697a0a913827e11cfabde2123b2b13730556310dd89eef1e9
#     HEAD_REF main
# )

get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
message("Using local source snapshot from ${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_VCPKG=ON
        -DBUILD_PACKAGE=NO
        -DBUILD_TEST_TOOL=NO
        -DBUILD_UNIT_TESTS=NO
        -DBUILD_FUNC_TESTS=NO
)

vcpkg_cmake_install()

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

