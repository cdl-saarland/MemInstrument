set(LLVM_NO_RTTI 1)

set(MEMINSTRUMENT_HEADER_FILES)
if (MSVC_IDE OR XCODE)
  file(GLOB_RECURSE MEMINSTRUMENT_HEADER_FILES "${MEMINSTRUMENT_SOURCE_DIR}/include/meminstrument/*.h")
endif ()

configure_file("../include/meminstrument/Definitions.h.in" "../include/meminstrument/Definitions.h" )

# add all source files of the project that should be linked together
add_meminstrument_library(meminstrument
  pass/MemInstrumentPass.cpp
  pass/ITargetGathering.cpp
  pass/ITargetFilters.cpp
  pass/WitnessGeneration.cpp
  pass/CheckGeneration.cpp
  pass/ITarget.cpp
  pass/Util.cpp
  pass/PerfData.cpp
  pass/WitnessGraph.cpp
  pass/DummyExternalChecksPass.cpp
  pass/ExternalChecksInterface.cpp
  instrumentation_mechanisms/InstrumentationMechanism.cpp
  instrumentation_mechanisms/DummyMechanism.cpp
  instrumentation_mechanisms/SplayMechanism.cpp
  instrumentation_mechanisms/NoopMechanism.cpp
  instrumentation_mechanisms/RuntimeStatMechanism.cpp
  instrumentation_policies/InstrumentationPolicy.cpp
  instrumentation_policies/BeforeOutflowPolicy.cpp
  instrumentation_policies/AccessOnlyPolicy.cpp
  instrumentation_policies/NonePolicy.cpp
  witness_strategies/WitnessStrategy.cpp
  witness_strategies/AfterInflowStrategy.cpp
  witness_strategies/NoneStrategy.cpp
  RegisterPasses.cpp
  Config.cpp
  lifetimekiller/LifeTimeKillerPass.cpp
)

# List external library dependencies that need to be linked to your library
# (see project/myproject level CMakelists to enforce that these libraries are found)

if (BUILD_SHARED_LIBS)
  target_link_libraries(meminstrument
    LLVMSupport
    LLVMCore
    LLVMTransformUtils
    LLVMAnalysis
    LLVMipo
    LLVMMC
  )
  link_directories(
    ${LLVM_LIBRARY_DIR}
  )
endif()

if(HAS_SQLITE3)
  target_link_libraries(meminstrument sqlite3)
endif()

if (MEMINSTRUMENT_USE_PMDA)
target_link_libraries(meminstrument checkoptimizer)
endif()

link_directories(
  ${LLVM_LIBRARY_DIR}
)

# Build a monolithic meminstrument.a and a thin module LLVMMEMINSTRUMENT.moduleext that links to
# that static library.
if (MSVC)
  # Add dummy target, because loadable modules are not supported on Windows
  add_custom_target(LLVMmeminstrument)
  set_target_properties(LLVMmeminstrument PROPERTIES FOLDER "meminstrument")
else ()
  # Specify the files clang needs for the pass manager
  add_meminstrument_loadable_module(LLVMmeminstrument
    pass/MemInstrumentPass.cpp
    pass/ExternalChecksInterface.cpp
    RegisterPasses.cpp
    Config.cpp
    lifetimekiller/LifeTimeKillerPass.cpp
  )

  target_link_libraries(LLVMmeminstrument meminstrument)

  set_target_properties(LLVMmeminstrument
    PROPERTIES
    LINKER_LANGUAGE CXX
    PREFIX "")
endif ()

if (TARGET intrinsics_gen)
  # Check if we are building as part of an LLVM build
  add_dependencies(meminstrument intrinsics_gen)
endif()

