message(STATUS "Configuring meminstrument...")

if(NOT DEFINED LLVM_MAIN_SRC_DIR)
  message(FATAL_ERROR "Meminstrument needs to be built as part of LLVM")
endif(NOT DEFINED LLVM_MAIN_SRC_DIR)

# ------------------------------------------------------------------------------
# Handle dependencies
# ------------------------------------------------------------------------------

set(MEMINSTRUMENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MEMINSTRUMENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Search for lifetimekiller
find_path(
  LIFETIMEKILLER_BASE "include/lifetimekiller/LifeTimeKillerPass.h"
  PATHS "${LLVM_MAIN_SRC_DIR}/projects"
  PATH_SUFFIXES "/lifetimekiller")
if(LIFETIMEKILLER_BASE)
  message(STATUS "Found LifeTimeKiller in ${LIFETIMEKILLER_BASE}")
else()
  message(FATAL_ERROR "LifeTimeKiller not found.")
endif()

message(STATUS "Looking for PMDA + PICO...")

find_path(
  PICO_BASE "include/PICO/PICO.h"
  PATHS "${LLVM_MAIN_SRC_DIR}/projects"
  PATH_SUFFIXES "/PICO")

set(PICO_AVAILABLE 0)
if(PICO_BASE)
  message(STATUS "Found PICO in ${PICO_BASE}")
  find_path(
    PMDA_BASE "include/PMDA/PMDA.h"
    PATHS "${LLVM_MAIN_SRC_DIR}/projects"
    PATH_SUFFIXES "/PMDA")

  if(PMDA_BASE)
    message(STATUS "Found PMDA in ${PMDA_BASE}")
    set(PICO_AVAILABLE 1)
  else()
    message(FATAL_ERROR "PICO found but not PMDA.")
  endif()

else()
  message(STATUS "PICO not found.")
endif()

# See if we have an installation of sqlite3, this is necessary for reading
# performance data.
find_path(SQLITE3_INCLUDE_PATH "sqlite3.h")

if(SQLITE3_INCLUDE_PATH)
  message(STATUS "Found sqlite3 in ${SQLITE3_INCLUDE_PATH}")
  if(NOT EXISTS ${SQLITE3_INCLUDE_PATH}/sqlite3.h)
    message(FATAL_ERROR "Cannot find sqlite3.h in ${SQLITE3_INCLUDE_PATH}.")
  endif()
  set(HAS_SQLITE3 1)
else()
  message(STATUS "sqlite3 not found. Reading performance data is disabled.")
  set(HAS_SQLITE3 0)
endif()

# Define the path to the run-time and provide a target that defines how to build
# it
set(MEMINSTRUMENT_RT_PATH "${MEMINSTRUMENT_SOURCE_DIR}/runtime")

add_custom_target(
   instrumentation-mechanisms
   COMMENT "Build the instrumentation mechanisms..."
   COMMAND make
   WORKING_DIRECTORY "${MEMINSTRUMENT_RT_PATH}"
)

# Allow to automatically build the LTO-enabled mechanisms; this will only work
# if gold is available
set(GOLD_PATH "${LLVM_LIBRARY_DIR}/LLVMgold.so")

add_custom_target(
   instrumentation-mechanisms-lto
   COMMENT "Build the lto-enabled instrumentation mechanisms..."
   COMMAND export LLVM_GOLD="${GOLD_PATH}" && make lto
   DEPENDS "${GOLD_PATH}"
   WORKING_DIRECTORY "${MEMINSTRUMENT_RT_PATH}"
)

set(RT_INCLUDE_PATH "${MEMINSTRUMENT_RT_PATH}/include")

set(CLANG_CMD "${LLVM_BINARY_DIR}/bin/clang")
set(MI_LIB_PATH "${LLVM_LIBRARY_DIR}/LLVMmeminstrument.so")
set(RT_LIBS_PATH "${MEMINSTRUMENT_RT_PATH}/lib")

configure_file(${MEMINSTRUMENT_SOURCE_DIR}/tools/mi_compile.sh.in
               ${MEMINSTRUMENT_BINARY_DIR}/tools/mi_compile.sh)

file(
  COPY ${MEMINSTRUMENT_BINARY_DIR}/tools/mi_compile.sh
  DESTINATION ${LLVM_BINARY_DIR}/bin
  FILE_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

# ------------------------------------------------------------------------------
# General build requirements
# ------------------------------------------------------------------------------

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(lib)
add_subdirectory(test)

# ------------------------------------------------------------------------------
# Add clang-format options for meminstrument
# ------------------------------------------------------------------------------

file(GLOB_RECURSE files *.h *.cpp)
set(check_format_depends)
set(update_format_depends)
set(i 0)
foreach(file IN LISTS files)
  add_custom_command(
    OUTPUT meminstrument-check-format${i}
    COMMAND clang-format -sort-includes -style=llvm ${file} | diff -u ${file} -
    VERBATIM
    COMMENT "Checking format of ${file}...")
  list(APPEND check_format_depends "meminstrument-check-format${i}")

  add_custom_command(
    OUTPUT meminstrument-update-format${i}
    COMMAND clang-format -sort-includes -i -style=llvm ${file}
    VERBATIM
    COMMENT "Updating format of ${file}...")
  list(APPEND update_format_depends "meminstrument-update-format${i}")

  math(EXPR i ${i}+1)
endforeach()

add_custom_target(meminstrument-check-format DEPENDS ${check_format_depends})
set_target_properties(meminstrument-check-format PROPERTIES FOLDER
                                                            "meminstrument")

add_custom_target(meminstrument-update-format DEPENDS ${update_format_depends})
set_target_properties(meminstrument-update-format PROPERTIES FOLDER
                                                             "meminstrument")

message(STATUS "meminstrument project configuration successful")
