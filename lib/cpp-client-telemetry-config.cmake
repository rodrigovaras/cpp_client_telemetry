include(CMakeFindDependencyMacro)

find_dependency(ZLIB)
find_dependency(unofficial-sqlite3)
find_dependency(CURL)

include("${CMAKE_CURRENT_LIST_DIR}/cpp-client-telemetry-targets.cmake")