# THE LIST OF PLUGINS AND THE CORRESPONDING SOURCE FILES
# ======================================================
set(BOGUSCONTROLFLOW_PLUGINS
    BogusControlFlow
    )

set(BogusControlFlow_SOURCES
    BogusControlFlow.cpp)


# show the list of plugins
message(STATUS "BogusControlFlow: ${BOGUSCONTROLFLOW_PLUGINS}")
# CONFIGURE THE PLUGIN LIBRARIES
# ==============================
foreach( plugin ${BOGUSCONTROLFLOW_PLUGINS} )
    # Create a library corresponding to 'plugin'
    add_library(
      ${plugin}
      SHARED
      ${${plugin}_SOURCES}
      )

    # Configure include directories for 'plugin'
    target_include_directories(
      ${plugin}
      PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    )


    target_link_libraries(
      ${plugin}
      "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>"
      )
endforeach()