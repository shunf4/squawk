find_path(Signal_INCLUDE_DIR NAMES signal/signal_protocol.h)
find_library(Signal_LIBRARY signal-protocol-c)
mark_as_advanced(Signal_INCLUDE_DIR Signal_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Signal REQUIRED_VARS Signal_LIBRARY Signal_INCLUDE_DIR)

if (Signal_FOUND AND NOT TARGET Signal::Signal)
  add_library(Signal::Signal UNKNOWN IMPORTED)
  set_target_properties(Signal::Signal PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    IMPORTED_LOCATION "${Signal_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Signal_INCLUDE_DIR}"
    )
endif ()
