.. _RunningSanitizers:

##################
Running Sanitizers
##################

.. contents::
    :local:

Overview
=========

Sanitizers allow developers to find any issues with their code at runtime
related to memory, threading or undefined behaviour.

Pre-requisites
==============

The following tooling is required:

- CMake 13.0 onwards (instructions for Ubuntu `here <https://apt.kitware.com/>`__)
- GCC-8 onwards


Switching to Sanitizer Build
============================

The following sanitizers modes are available:

- Address
- Memory (Not implemented yet, requires clang support)
- Thread
- Undefined

It's recommended to build in *RelWithDebInfo* mode, CMake will automatically
switch to -O1 and append extra flags for stack traces to work correctly whilst
preserving high performance

If required, a build can be completed in debug mode too, however the
performance penalty will be pretty severe.

Command Line
------------

First, delete *CMakeCache.txt* if your system compiler is GCC 7 or below
(you can check with *gcc -v*).

To change to a sanitized build navigate to your build folder and execute the
following:

    .. code-block:: sh

        cmake *path_to_src* -DUSE_SANITIZER=*Mode* -DCMAKE_BUILD_TYPE=RelWithDebInfo

If you need to specify a different compiler too (for example if your system
default is GCC-7)

    .. code-block:: sh

        CC=gcc-8 CXX=g++-8 cmake *path_to_src* -DUSE_SANITIZER=*Mode* -DCMAKE_BUILD_TYPE=RelWithDebInfo


For example, to switch to an address sanitizer build the following can be used:

    .. code-block:: sh

        cmake *path_to_src* -DUSE_SANITIZER=Address -DCMAKE_BUILD_TYPE=RelWithDebInfo

CMake GUI
---------

- Delete the cache if your system compiler is GCC 7 or below (you can check
  with *gcc -v*). Hit configure and click specify native compilers to *gcc-8*
  and *g++-8*
- Change *CMAKE_BUILD_TYPE* to *RelWithDebInfo* (or *Debug*)
- Change *USE_SANITIZERS* to any of the options in the drop down
- Hit generate and rebuild the project

Running Tests
=============

Several upstream linked libraries currently contain leaks which we cannot
resolve (or have been resolved but not appeared downstream).

We can suppress warnings in the address sanitizer by setting environment
variables in the console before running in each mode.

Address Sanitizer
-----------------

Run the following code to set the sanitizers options, being sure to substitute
the path to your Mantid source directly:

    .. code-block:: sh

        export ASAN_OPTIONS="verify_asan_link_order=0:detect_stack_use_after_return=true:halt_on_error=false:suppressions=*path_to_mantid*/buildconfig/Sanitizer/Address.supp"
        export LSAN_OPTIONS="suppressions=*path_to_mantid*/buildconfig/Sanitizer/Leak.supp"

For example, if Mantid was checked out in the home directory of user *abc* in a
folder called mantid it would be:

    .. code-block:: sh

        export ASAN_OPTIONS="verify_asan_link_order=0:detect_stack_use_after_return=true:halt_on_error=false:suppressions=/home/abc/mantid/buildconfig/Sanitizer/Address.supp"
        export LSAN_OPTIONS="suppressions=/home/abc/mantid/buildconfig/Sanitizer/Leak.supp"

All code executed in **the shell where the previous commands were run in**
will now be sanitized correctly.

Instrumenting Python (Advanced)
-------------------------------

Currently any code started in Python (i.e. Python Unit Tests) will not pre-load
ASAN instrumentation. This can be split into two categories:

- Code which uses Python only components: Not worth instrumenting as any
  issues will be upstream. This also will emit an error if
  *verify_asan_link_order* is set to true, as we technically haven't
  instrumented anything (unless you have a sanitized Python build)
- Code which uses Mantid C++ components: This can be instrumented, but
  (currently) isn't by default, as the user has to determine the *LD_PRELOAD*
  path.

If you need / want to profile C++ components which are triggered from Python
the following steps should setup your environment:

    .. code-block:: sh

        # Get the path to your linked ASAN
        ldd bin/KernelTest | grep "libasan"
        export LD_PRELOAD=/usr/lib/path_to/libasan.so.x

        # You may want to re-run the ASAN_OPTIONS export dropping
        # the verify to make sure that the C++ component is being instrumented:

        export ASAN_OPTIONS="detect_stack_use_after_return=true:halt_on_error=false:suppressions=*path_to_mantid*/buildconfig/Sanitizer/Address.supp"


Common Problems
===============

Library Leaks Appearing
-----------------------

Check that you have correctly spelt *suppressions* as there will be no warnings
for typos. A good check is to put some random characters in the .supp files,
which will cause all tests to fail if it's begin read.

Any new third party memory leaks need to go into *Leaks.supp* not
*Address.supp* (which should ideally be completely empty) to be suppressed.

ASAN was not the first library loaded
--------------------------------------

This can appear when running Python tests, as the executable is not build
with instrumentation. To avoid this warning ensure that
*verify_asan_link_order=0* is set in your environment and that you are
using GCC 8 onwards.
