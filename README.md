# Instrumentation passes for enforcing memory safety.

## Getting Started

### 0. Requirements

  * a modern Linux operating system with a modern C++ compiler (e.g. `g++`, other operating systems might also work but are not officially supported)
  * `git`
  * `cmake` (version as required by LLVM)
  * a supported build system, e.g. `ninja` (recommended) or GNU make
  * the built C part of the implementation, the [instrumentation-mechanisms](https://gitlab.cs.uni-saarland.de/cdl/safe-c/instrumentation-mechanisms)

### 1. Setting up LLVM

Start in your project root folder. Clone the [LLVM sources from the monorepo](https://github.com/llvm/llvm-project) as `llvm-project` and checkout the required branch (currently supported: `release/12.x`).

### 2. Setting up meminstrument

Clone this repository and the [`lifetimekiller`](https://gitlab.cs.uni-saarland.de/cdl/safe-c/lifetimekiller) pass into `llvm-project/llvm/projects`.

### 3. Building LLVM+Clang+meminstrument

Create a new folder `build` in your project root folder and `cd` into it. Create build files via `cmake` with the following command:

```
cmake -G <build tool> -DLLVM_ENABLE_PROJECTS='clang' -DCMAKE_INCLUDE_PATH=</path/to/instrumentation-mechanisms> <additional flags> ../llvm-project/llvm/
```

where `<build tool>` is your preferred build tool (e.g. `Ninja`).
Interesting additional flags for development are:

  * `-DPICO_USE_MEMINSTRUMENT=1` to tell [PICO](https://gitlab.cs.uni-saarland.de/cdl/safe-c/PICO) that it will be used by meminstrument (only if you also build PICO)
  * `-DLLVM_ENABLE_ASSERTIONS=1` to enforce assertions in the code
  * `-DCMAKE_BUILD_TYPE=Debug` to enable a debug build for better error messages and debugging capabilities
  * `-DLLVM_PARALLEL_LINK_JOBS=<n>` to limit the number of concurrent link jobs to `<n>`. This should be rather low for systems with less than 8GB of RAM as linking `clang` can require considerable amounts of memory (especially when building a debug build)
  * `-DLLVM_TARGETS_TO_BUILD=X86` to reduce build time by building only the X86 backend
  * `-DCMAKE_INSTALL_PREFIX=</path/to/install/directory>` to specify an installation directory for llvm

When the build files are generated, use your build system to build the project (e.g. by typing `ninja`). This might take a while.

Note that an LLVM debug build currently requires around 60GB of memory.

### 4. Build Targets

`meminstrument` defines additional build targets to use with the build system:

  * `check-meminstrument` to run the `meminstrument` test suite
  * `meminstrument-update-format` to run `clangformat` on all `meminstrument` source files

## Using the Instrumentation Passes

### Basic usage

... with clang:

```
clang -Ox -Xclang -load -Xclang <path to LLVM build>/lib/LLVMmeminstrument.so
```
Note that at least optimization level `-O1` is required, without it the instrumentation will not be run.

... with opt:

```
opt -load <path to LLVM build>/lib/LLVMmeminstrument.so -meminstrument
```

Various different instrumentations are available, the default is `splay`. To run the others, use:

```
-mi-config=<instrumentation>
```

The most interesting options for `<instrumentation>` are `splay`, `lowfat` and `softbound`.

Note: If you want to pass arguments for the instrumentation to `clang`, add `-mllvm` in front of each of them (e.g. `-mllvm -mi-config=lowfat -mllvm -mi-mode=setup`).

### Linking

The compiled program needs to be linked against the C part of the instrumentation.
Use

```
-L</path/to/instrumentation-mechanisms>/lib -ldl -l:lib<instrumentation>.a
```

to link the library.

For SoftBound(CETS) use:

```
-L</path/to/instrumentation-mechanisms>/lib -lm -lrt -lsoftboundcets_rt -luuid -ldl -lcrypt
```

### Full list of available options

The available command line flags for the instrumentations can be found under "MemInstrument Options" in the help of `opt`.

```
opt -load <path to LLVM build>/lib/LLVMmeminstrument.so --help
```
