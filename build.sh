#!/bin/bash
set -e

# Repos URLs
LLVMSRC=https://github.com/llvm/llvm-project.git
MISRC=https://github.com/cdl-saarland/MemInstrument.git
BINUTILS=git://sourceware.org/git/binutils-gdb.git

# Basic structure
INSTALL_ROOT="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
MIREPOS=$INSTALL_ROOT/meminstrument-repos
MIBUILD=$INSTALL_ROOT/meminstrument-build
mkdir -p $MIREPOS $MIBUILD

# Setup binutils
cd $MIREPOS
git clone $BINUTILS -b gdb-12-branchpoint --depth 1

# Clone LLVM 12
git clone $LLVMSRC -b release/12.x --depth 1

# Check out MemInstrument
cd llvm-project/llvm/projects
git clone $MISRC meminstrument
cd meminstrument
git submodule init && git submodule update

# Build with meminstrument enabled
LLVMBUILD=$MIBUILD/llvm-meminstrument-build
mkdir -p $LLVMBUILD
cd $LLVMBUILD
cmake -G Ninja -D LLVM_ENABLE_PROJECTS="clang" -D CMAKE_BUILD_TYPE=Release -D LLVM_TARGETS_TO_BUILD=X86 -D LLVM_BINUTILS_INCDIR=$MIREPOS/binutils-gdb/include $MIREPOS/llvm-project/llvm
ninja
