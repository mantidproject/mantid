# Running Sanitizers

::: {.contents local=""}
:::

## Overview

Sanitizers allow developers to find any issues with their code at
runtime related to memory, threading or undefined behaviour.

## Pre-requisites

The following tooling is required for the GCC compiler:

- GCC-8 onwards
- Clang 3.2 onwards

Microsoft Visual Studio 2019 provides an (experimental) address
sanitizer that applies to x64 targets in version 16.7 and later

## Switching to Sanitizer Build

The following sanitizers modes are available:

- Address
- Thread (GCC only)
- Undefined (GCC only)

It's recommended to build in *RelWithDebInfo* mode, CMake will
automatically switch to -O1 and append extra flags for stack traces to
work correctly whilst preserving high performance

If required, a build can be completed in debug mode too, however the
performance penalty will be pretty severe.

### Command Line

First, delete *CMakeCache.txt* if using GCC and your system compiler is
GCC 7 or below (you can check with *gcc -v*).

To change to a sanitized build navigate to your build folder and execute
the following if you're using a single-config generator (eg Ninja):

``` sh
cmake *path_to_src* -DUSE_SANITIZER=*Mode* -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

...or the following with a multi-config generator (eg Visual Studio):

``` sh
cmake *path_to_src* -DUSE_SANITIZER=*Mode*
```

If you are using GCC and need to specify a different compiler too (for
example if your system default is GCC-7)

``` sh
CC=gcc-8 CXX=g++-8 cmake *path_to_src* -DUSE_SANITIZER=*Mode* -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

For example, to switch to an address sanitizer build the following can
be used:

``` sh
cmake *path_to_src* -DUSE_SANITIZER=Address -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### CMake GUI

- If using GCC, delete the cache if your system compiler is GCC 7 or
  below (you can check with *gcc -v*)
- Hit configure
- If using GCC, click specify native compilers to *gcc-8* and *g++-8*
- If using a single-config generator, change *CMAKE_BUILD_TYPE* to
  *RelWithDebInfo* (or *Debug*)
- Change *USE_SANITIZERS* to any of the options in the drop down
- Hit configure again, then generate and rebuild the project

## Running Tests

Several upstream linked libraries currently contain leaks which we
cannot resolve (or have been resolved but not appeared downstream).

We can suppress warnings in the address sanitizer by setting environment
variables in the console before running in each mode.

## Visual Studio Address Sanitizer

The Visual Studio address sanitizer raises exceptions with code
0xC0000005 as part of its normal operation. The exceptions are handled
but they cause the debugger to stop. These exceptions need to be ignored
in the Debug, Windows, Exception Settings dialog

Genuine sanitizer issues cause an unhandled exception and the debugger
will stop at the relevant unitline.

Some parts of the Mantid code (eg H5Util.cpp) don't compile when the
address sanitizer is enabled and the /O2 optimisation flag is used (in
RelWithDebInfo). This flag is switched to /O1 in order to improve stack
trace information (see above) and this fortunately removes the
compilation errors.

The following path (or equivalent for your Visual Studio version) needs
to be added to your path environment variable in order for Visual Studio
to find some .lib files that are used by the sanitizer and also to
locate a symbolizer exe that is required for useful error messages:

``` sh
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.27.29110\bin\Hostx64\x64
```

Additional information is available on this [web
page](https://devblogs.microsoft.com/cppblog/asan-for-windows-x64-and-debug-build-support/).

## Advanced Details

Most developers do not need to read this, but it's good for those who
want to know what's happening

CMake substitutes in various flags for the address sanitizer builds to
setup suppressions etc... this is the equivalent of doing the following
in a local shell:

``` sh
export ASAN_OPTIONS="verify_asan_link_order=0:detect_stack_use_after_return=true:halt_on_error=false:suppressions=*path_to_mantid*/tools/Sanitizer/Address.supp"
export LSAN_OPTIONS="suppressions=*path_to_mantid*/tools/Sanitizer/Leak.supp"
```

All code executed which is executed in that shell will now be sanitized
correctly. To save developers effort the CXX_ADD_TEST macro (in
FindCxxTest.cmake) will append these environment variables on a
developers behalf.

### Instrumenting Python (Advanced)

Currently any code started in Python (i.e. Python Unit Tests) will not
pre-load ASAN instrumentation. This can be split into two categories:

- Code which uses Python only components: Not worth instrumenting as any
  issues will be upstream. This also will emit an error if
  *verify_asan_link_order* is set to true, as we technically haven't
  instrumented anything (unless you have a sanitized Python build)
- Code which uses Mantid C++ components: This can be instrumented, but
  (currently) isn't by default, as the user has to determine the
  *LD_PRELOAD* path.

If you need / want to profile C++ components which are triggered from
Python the following steps should setup your environment:

``` sh
# Get the path to your linked ASAN
ldd bin/KernelTest | grep "libasan"
export LD_PRELOAD=/usr/lib/path_to/libasan.so.x

# leak detection should only show the largest 25 leaks
export LSAN_OPTIONS="max_leaks=25"

# You may want to re-run the ASAN_OPTIONS export dropping
# the verify to make sure that the C++ component is being instrumented:
# log_path is the prefix for the file the results are written to

export ASAN_OPTIONS="detect_stack_use_after_return=true:halt_on_error=false:log_path=asan:suppressions=*path_to_mantid*/tools/Sanitizer/Address.supp"
```

## Common Problems

### Library Leaks Appearing

Check that you have correctly spelt *suppressions* as there will be no
warnings for typos. A good check is to put some random characters in the
.supp files, which will cause all tests to fail if it's begin read.

Any new third party memory leaks need to go into *Leaks.supp* not
*Address.supp* (which should ideally be completely empty) to be
suppressed.

### ASAN was not the first library loaded

This can appear when running Python tests, as the executable is not
build with instrumentation. To avoid this warning ensure that
*verify_asan_link_order=0* is set in your environment and that you are
using GCC 8 onwards.
