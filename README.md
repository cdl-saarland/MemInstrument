# The MemInstrument framework

The MemInstrument framework offers several memory safety instrumentations implemented as compiler extension for LLVM/Clang 12.
When an instrumentation is enabled, the compiler inserts code that checks memory accesses for their validity.
The inserted code is then executed at the runtime of the program, and reports an error if a memory safety violation is detected.

A high-level project description can be found [here](https://compilers.cs.uni-saarland.de/projects/meminstrument/).

Find details on the provided memory safety instrumentations in `runtime/README.md`.

## Getting Started

### Requirements

LLVM and our project require various C/C++ related packages (`cmake`, `ninja`, a C++ compiler, `ctags`, the `uuid` library+headers).

On a plain Ubuntu Server 20.04 LTS, the following commands will set up everything you need to build MemInstrument:
```
# Dependencies for LLVM
apt-get install cmake ninja-build build-essential
# Dependencies for MemInstrument
apt-get install uuid-dev universal-ctags
```

### Building LLVM and MemInstrument

The build script clones several repositories and creates an LLVM release build.
Note that you can expect a memory usage of around 6GB in total.

Download `build.sh` to the folder where you want MemInstrument to live.

Execute it with

`./build.sh`

Afterwards, you will have two folders:

* `meminstrument-repos`, where the LLVM, binutils, and MemInstrument sources are
* `meminstrument-build`, which contains your self-built LLVM with MemInstrument

### Note

`Binutils` are not strictly required to build MemInstrument.
However, especially the performance of SoftBound heavily depends on the usage of LTO (you can expect multiple times the regular runtime without LTO).
Therefore, we build the lto-ready runtime library by default, which requires `binutils`.

## Using the Instrumentation Passes

### Basic usage

... with (the just built) clang:

```
clang -O<x> -Xclang -load -Xclang <path to LLVM build>/lib/LLVMmeminstrument.so
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
For `lowfat`, the additional flag `-mcmodel=large` is required (or you have to disable the global variable extension).

Note: If you want to pass arguments for the instrumentation to `clang`, add `-mllvm` in front of each of them (e.g. `-mllvm -mi-config=lowfat -mllvm -mi-mode=setup`).

### Linking

The compiled program needs to be linked against the C part of the instrumentation.
Use

```
-L<path to LLVM source>/llvm/projects/meminstrument/runtime/lib -ldl -l:lib<instrumentation>.a
```

to link the library. For SoftBound, additionally append following linker flags: ` -luuid -lm -lrt -lcrypt`

### Link-Time Optimizations (LTO)

In order to use LTO, append `-flto -fuse-ld=gold` to your linker flags.
Instead of the libraries in `lib`, use those in `lib/lto`.

### Full list of available options

The available command line flags for the instrumentations can be found under "MemInstrument Options" in the help of `opt`.

```
opt -load <path to LLVM build>/lib/LLVMmeminstrument.so --help
```

### Testing

MemInstrument uses the built-in testing framework of LLVM. To execute the tests, use

```
cd <path to LLVM build>
ninja check-meminstrument
```

## Contributions

The framework was set up by **Fabian Ritter** and further developed by **Tina Jung**.

**Fabian Ritter** contributed the splay instrumentation, as well as various statistics and testing mechanisms.

The lowfat implementation (heap protection) was contributed by **Philip Gebel** and extended by the stack and global variable protection by **Tina Jung**.

The SoftBound implementation is by **Tina Jung**.
