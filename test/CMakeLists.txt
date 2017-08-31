# reference file: llvm/test/CMakeLists.txt

find_path(MEMINSTRUMENT_RT_LIB_PATH libsplay.a)
if(NOT ${MEMINSTRUMENT_RT_LIB_PATH_FOUND})
  message(STATUS "Cannot find meminstrument runtime libs")
endif()

# Tell lit to auto-generate lit.site.cfg from the hand-written lit.site.cfg.in
# This makes user specific paths which are already known to cmake also
# available for lit.cfg without hardcoding them
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  )

# The tests are run in the binary dir, this is also where the generate site.cfg
# can be found.
add_lit_testsuite(check-meminstrument "Running the meminstrument lit tests"
  ${CMAKE_CURRENT_BINARY_DIR}
    PARAMS meminstrument_site_config=${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  )
set_target_properties(check-meminstrument PROPERTIES FOLDER "Tests")


# Add a legacy target spelling
add_custom_target(meminstrument-test)
set_target_properties(meminstrument-test PROPERTIES FOLDER "meminstrument")
add_dependencies(meminstrument-test check-meminstrument)
