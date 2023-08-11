include(CMakeFindDependencyMacro)

find_dependency(ZLIB)
find_dependency(unofficial-sqlite3)

include("${CMAKE_CURRENT_LIST_DIR}/cpp-client-telemetry-targets.cmake")