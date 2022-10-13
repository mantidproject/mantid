.. _clion-ref:

=====
CLion
=====

.. contents::
  :local:

Installing CLion
################

Please note that these instructions only work when using a Ninja generator from a Windows or Linux operating system.

You will also need to have Visual Studio installed on windows.

If you haven't installed CLion yet do that now, CLion can be installed from `here <https://jetbrains.com/clion/download/>`_.

Setup for a CLion Build
#######################

To build using a Ninja generator from the CLion IDE, follow these instructions from within CLion:

To set up your toolchain:

- Navigate to `File > Settings > Build, Execution, Deployment > Toolchains`
- Create a new `System` toolchain using the `+` icon and call it `Default`
- Edit the CMake field to point to your conda installed `cmake`, e.g. `/path/to/mambaforge/envs/mantid-developer/bin/cmake`
- Edit the Build Tool field to point to your conda installed `ninja`, e.g. `/path/to/mambaforge/envs/mantid-developer/bin/ninja`
- On Linux, `Let CMake detect` the C Compiler and C++ Compiler.
- On Windows, the C Compiler and C++ Compiler should be directed at the same `cl.exe` in your Visual Studio installation, e.g. `C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe`

To set up CMake:

- Navigate to `File > Settings > Build, Execution, Deployment > CMake`
- On Linux, set your Build type to be `Debug`. On Windows, set your build type to be `DebugWithRelRuntime`
- Set your Toolchain to be the `Default` toolchain that you just created
- Set your generator to be `Ninja`
- Set your cmake options to be `--preset=linux` or `--preset=win-ninja`
- Set the build directory to the `build` directory if it is not the default (you'll need to use the full path if its outside the source directory)
- The configurations drop-down at the top should show all of the build targets. If not, the CMake project is probably not loaded. Open the root `CMakeLists.txt` file in a tab and there should be a `Load CMake Project` option at the top right. Click it and the configurations should be populated.

Building with CLion
###################

- To build all targets, navigate to `Build > Build All in 'Debug'`. Check that the build command displayed in the Messages window is running the correct cmake executable from your conda installation.
- To build a specific target, select it in the configurations drop-down menu and click the hammar icon next to it.

Debugging with CLion
####################


