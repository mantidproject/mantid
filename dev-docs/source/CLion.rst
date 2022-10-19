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

- Navigate to ``File > Settings > Build, Execution, Deployment > Toolchains``
- Create a new ``System`` toolchain using the ``+`` icon and call it ``Default``
- Edit the CMake field to point to your conda installed ``cmake``, e.g.
   - On Linux: ``/path/to/mambaforge/envs/mantid-developer/bin/cmake``
   - On Windows: ``/path/to/mambaforge/envs/mantid-developer/Library/bin/cmake.exe``
- Edit the Build Tool field to point to your conda installed ``ninja``, e.g.
   - On Linux: ``/path/to/mambaforge/envs/mantid-developer/bin/ninja``
   - On Windows: ``/path/to/mambaforge/envs/mantid-developer/Library/bin/ninja.exe``
- For the C Compiler and C++ Compiler fields,
   - On Linux: choose ``Let CMake detect``
   - On Windows: direct them both at the same ``cl.exe`` in your Visual Studio installation, e.g. ``C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/bin/Hostx64/x64/cl.exe``

To set up CMake:

- Navigate to ``File > Settings > Build, Execution, Deployment > CMake``
- Edit the Build type field by either selecting an option, or typing in a string,
   - On Linux: ``Debug``
   - On Windows: ``DebugWithRelRuntime``
- Set your Toolchain to be the ``Default`` toolchain that you just created
- Set your generator to be ``Ninja``
- Edit your cmake options to be,
   - On Linux: ``--preset=linux``
   - On Windows: ``--preset=win-ninja``
- Set the build directory to the ``build`` directory if it is not the default (you'll need to use the full path if its outside the source directory)
- The configurations drop-down at the top should show all of the build targets. If not, the CMake project is probably not loaded. Go to ``File > Reload CMake Project``. The configurations should be populated

Building with CLion
###################

- To build all targets, navigate to ``Build > Build All in 'Debug'``. Check that the build command displayed in the Messages window is running the correct cmake executable from your conda installation.
- To build a specific target, select it in the configurations drop-down menu and click the hammar icon next to it.

If this fails, you may need to open CLion from a terminal with your conda environment activated:

- Open a terminal and run ``conda activate mantid-developer``
- Launch CLion through that terminal with ``<CLION_INSTALL>/bin/clion.sh`` for Linux or ``<CLION_INSTALL>/bin/clion.bat`` for Windows

It is also useful to have your terminals in CLion to run with this environment:

- In your ``home`` directory create a file named ``.clionrc`` and open in your favourite text editor, adding these lines:

  ```
  source ~/.bashrc
  source ~/mambaforge/bin/activate mantid-developer
  ```

- Start CLion using the above steps
- Navigate to ``File > Settings > Tools > Terminal``
- To the end of the ``Shell path`` option, add ``--rcfile ~/.clionrc``

Debugging with CLion
####################

To debug workbench, you'll need to edit the ``workbench`` CMake Application configuration.

- Set the executable to be the ``python.exe`` in your conda installation, e.g. ``/path/to/mambaforge/envs/mantid-developer/bin/python.exe``
- Set the program arguments to be ``workbench --single-process`` on Linux, or ``workbench-script.pyw --single-process`` on Windows
- Set the working directory to be the full path to your ``build/bin`` directory

You should now be able to set breakpoints and start debugging by clicking the bug icon.

