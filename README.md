# Instrumentation passes for enforcing memory safety.

## Getting Started

### 0. Requirements

  * a modern Linux operating system with a modern C++ compiler (e.g. `g++`, other operating systems might also work but are not officially supported)
  * `git`
  * `cmake` (version >= ??)
  * a supported build system, e.g. `ninja` or GNU make

### 1. Setting up LLVM

Start in your project root folder. Clone the [LLVM sources](https://github.com/llvm-mirror/llvm) as `llvm`. Next, clone the [Clang sources](https://github.com/llvm-mirror/clang) into `llvm/tools` and the [Compiler-RT sources](https://github.com/llvm-mirror/compiler-rt) into `llvm/projects`. Make sure to checkout the same branch/commit in all three of these repositories (currently supported: `release_40`).

### 2. Setting up meminstrument

Clone this repository into `llvm/projects`.

### 3. Building LLVM+Clang+meminstrument


Create a new folder `build` in your project root folder and `cd` into it. Create build files via `cmake` with the following command:

```
cmake -G <build tool> <additional flags> -DLLVM_TOOL_COMPILER_RT_BUILD=1 ../llvm/
```

where `<build tool>` is your preferred build tool (e.g. `Ninja`).
Interesting additional flags for development are:

  * `-DCMAKE_INCLUDE_PATH=</path/to/instrumentation-mechanisms/lib>` to allow finding runtime libraries for the implemented instrumentation mechanisms from a separate repository. (Omitting can make tests fail.)
  * `-DLLVM_ENABLE_ASSERTIONS=1` to enforce assertions in the code
  * `-DCMAKE_BUILD_TYPE=Debug` to enable a debug build for better error messages and debugging capabilities
  * `-DLLVM_PARALLEL_LINK_JOBS=<n>` to limit the number of concurrent link jobs to `<n>`. This should be rather low for systems with less than 8GB of RAM as linking `clang` can require considerable amounts of memory (especially when building a debug build)
  * `-DLLVM_TARGETS_TO_BUILD=X86` to reduce build time by building only the X86 backend

When the build files are generated, use your build system to build the project (e.g. by typing `ninja`). This might take a while.

### 4. Build Targets

`meminstrument` defines additional build targets to use with the build system:

  * `check-meminstrument` to run the `meminstrument` test suite
  * `meminstrument-update-format` to run `clangformat` on all `meminstrument` source files

### 5. Using the Instrumentation Passes

TODO
