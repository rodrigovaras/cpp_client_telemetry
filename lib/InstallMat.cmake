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
