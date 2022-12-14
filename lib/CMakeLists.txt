# add all source files of the project that should be linked together
add_library(
  meminstrumentobjlib OBJECT
  pass/MemInstrumentPass.cpp
  pass/ITargetGathering.cpp
  pass/WitnessGeneration.cpp
  pass/CheckGeneration.cpp
  pass/ITarget.cpp
  pass/Setup.cpp
  pass/Util.cpp
  pass/WitnessGraph.cpp
  optimizations/AnnotationBasedRemovalPass.cpp
  optimizations/DominanceBasedCheckRemovalPass.cpp
  optimizations/ExampleExternalChecksPass.cpp
  optimizations/HotnessBasedCheckRemovalPass.cpp
  optimizations/OptimizationInterface.cpp
  optimizations/OptimizationRunner.cpp
  optimizations/PerfData.cpp
  instrumentation_mechanisms/InstrumentationMechanism.cpp
  instrumentation_mechanisms/SleepMechanism.cpp
  instrumentation_mechanisms/SplayMechanism.cpp
  instrumentation_mechanisms/LowfatMechanism.cpp
  instrumentation_mechanisms/ExampleMechanism.cpp
  instrumentation_mechanisms/RuntimeStatMechanism.cpp
  instrumentation_mechanisms/SoftBoundMechanism.cpp
  instrumentation_mechanisms/softbound/InternalSoftBoundConfig.cpp
  instrumentation_mechanisms/softbound/RunTimePrototypes.cpp
  instrumentation_mechanisms/softbound/SoftBoundWitness.cpp
  instrumentation_policies/InstrumentationPolicy.cpp
  instrumentation_policies/BeforeOutflowPolicy.cpp
  instrumentation_policies/AccessOnlyPolicy.cpp
  instrumentation_policies/NonePolicy.cpp
  instrumentation_policies/PointerBoundsPolicy.cpp
  witness_strategies/WitnessStrategy.cpp
  witness_strategies/AfterInflowStrategy.cpp
  witness_strategies/NoneStrategy.cpp
  RegisterPasses.cpp
  Config.cpp)

if(PICO_AVAILABLE)
  target_include_directories(
    meminstrumentobjlib BEFORE
    PUBLIC ${MEMINSTRUMENT_BINARY_DIR}/../PICO/include
           $<BUILD_INTERFACE:${PICO_BASE}/include>
           $<BUILD_INTERFACE:${PMDA_BASE}/include>
           $<BUILD_INTERFACE:${ISL_INCLUDE_DIR}>)
endif()

target_include_directories(
  meminstrumentobjlib BEFORE
  PUBLIC $<BUILD_INTERFACE:${MEMINSTRUMENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${MEMINSTRUMENT_BINARY_DIR}/include>
         $<BUILD_INTERFACE:${RT_INCLUDE_PATH}> $<INSTALL_INTERFACE:include>)

# LLVM globally defines whether RTTI + exceptions should be used, stick to that.
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(meminstrumentobjlib PRIVATE -fno-exceptions -fno-rtti)
endif()

target_compile_options(
  meminstrumentobjlib PRIVATE -fno-common -Woverloaded-virtual -Wno-long-long
                              -Wall -Wextra -pedantic)

add_dependencies(meminstrumentobjlib intrinsics_gen)

# Make sure the instrumentation mechanisms are built before meminstrument, they
# provide auto-generated header files required to built meminstrument

if(LLVM_BINUTILS_INCDIR)
  # Build the LTO-enabled stuff if possible
  add_dependencies(meminstrumentobjlib instrumentation-mechanisms-lto)
else()
  add_dependencies(meminstrumentobjlib instrumentation-mechanisms)
endif()

configure_file("../include/meminstrument/Definitions.h.in"
               "../include/meminstrument/Definitions.h")

# This function exports properties of your library and defines target specifics.
# We will always build two libraries. This function is used to apply the effects
# on both libraries consistently.
function(setup_meminstrument_library name)

  # List external library dependencies that need to be linked to your library
  # (see project/myproject level CMakelists to enforce that these libraries are
  # found)
  if(PICO_AVAILABLE)
    # TODO we currently have a cyclic dependency between PICO and meminstrument,
    # this will fail in a shared lib build.
    target_link_libraries(${name} PUBLIC PMDA PICO)
  endif()

  if(HAS_SQLITE3)
    target_link_libraries(${name} PUBLIC sqlite3)
  endif()

  set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX PREFIX "")

  install(
    TARGETS ${name}
    EXPORT LLVMExports
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX}
    INCLUDES
    DESTINATION include)
  set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS ${name})

endfunction(setup_meminstrument_library)

# The generated object files can now be used to create the kind of library
# determined by BUILD_SHARED_LIBS (defined outside of the project) and the
# MODULE library.
add_library(meminstrument $<TARGET_OBJECTS:meminstrumentobjlib>)
setup_meminstrument_library(meminstrument)

add_library(LLVMmeminstrument MODULE $<TARGET_OBJECTS:meminstrumentobjlib>)
setup_meminstrument_library(LLVMmeminstrument)

if(BUILD_SHARED_LIBS)
  link_directories(${LLVM_LIBRARY_DIR})
  llvm_map_components_to_libnames(llvm_libs all)
  target_link_libraries(meminstrument PUBLIC ${llvm_libs})
endif()
