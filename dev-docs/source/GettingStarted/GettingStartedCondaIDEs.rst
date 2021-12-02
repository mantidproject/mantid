.. _GettingStartedIDEs:

==================================
Getting Started with IDEs in Conda
==================================

.. contents::
  :local:

.. toctree::
   :hidden:

PyCharm
#######
The PyCharm documentation is up to date for Conda here: :ref:`PyCharm`

CLion
#####
Before opening/installing CLion, ensure you have completed the :ref:`GettingStarted` instructions.

Installation advice
-------------------
* Linux Only: Ensure that you perform all terminal instructions in the mantid-developer conda environment
* Follow these instructions for installation: https://www.jetbrains.com/help/clion/installation-guide.html#standalone
* Linux Only: Ensure you have added CLion to the desktop using the menu option Tools -> Create Desktop Entry...

Building with CLion
-------------------
* Open CLion
* If no project is selected, then select the mantid source clone
* Wait for CLion to index (you can see progress via the bottom of the CLion Window) and configure CMake again (Should be evident from the terminal that opens at the bottom of CLion)
* Go to menu option File -> Settings -> Build, Execution, Deployment -> CMake, add all of the CMake Options from your "CMakePresets.json" in the root of mantid source, to the "CMake Options" field in the options you have opened, make sure to add -D before each option for example: "-DCONDA_ENV=true", in addition to all the added options, ensure you add your generator after -G, for example on Linux is is "-G Ninja"
* An example of CMake Options is:

.. code-block:: sh

    -G Ninja
    -DCMAKE_PREFIX_PATH=$env{CONDA_PREFIX}
    -DCONDA_ENV=true
    -DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH=FALSE
    -DCMAKE_FIND_USE_CMAKE_ENVIRONMENT_PATH=FALSE
    -DCMAKE_BUILD_TYPE=Debug
    -DCMAKE_CXX_FLAGS_DEBUG="-g -O0"

* Change the "build directory" option to be the same name as your build folder, default is "build", if it is within the same directory as your source directory
* Change the "build options" option to be "-- -j16" where 16 is the number of logical cores your PC can use for compiling
* Hit apply, and then ok to close the options window
* Wait for CLion to re-index and re-run CMake the terminal at the bottom displays it's progress
* If a message appears along the lines of: "CMake Error: CMake was unable to find a build program corresponding to "Ninja"", it means it couldn't find the program because either your conda environment hasn't been started or you haven't installed the dependencies, a good fix if you are running in a conda environment, is to redo the CMake part of the :ref:`GettingStarted` instructions you did earlier.
* If you were successful, then you should be able to click the green hammer button to build, and select any of our CMake targets to build.

Debugging Workbench with CLion
------------------------------

* To debug workbench ensure you are using the configuration called "workbench | Debug", you can select the configuration using the drop down in the top right of the window.
* Before it will successfully utilise breakpoints we need to edit the configuration of "workbench | Debug", open the configuration menu by clicking Run -> Edit Configurations...
* In the now open windows, ensure you have selected the workbench configuration, and set the Executable option as the python executable from your conda environment, the path to this executable is in "{MAMBAFORGE_INSTALL_DIR}/envs/mantid-developer-bin/python".
* Set the program arguments to your compiled workbench executable which is in your "{BUILD_DIR}/bin/workbench"
* Set the environment variables options for "LD_PRELOAD" to your jemalloc dependency, an example is "{MAMBAFORGE_INSTALL_DIR}/envs/mantid-developer/lib/libjemalloc.so.2", next set your environment variable for "PYTHONPATH" to the build directories bin folder for example: "{BUILD_DIR}/bin"
* Now you should be able to build only the workbench target and run workbench, if you haven't already built the Mantid Framework, then you need to add a "Before Launch" step, in the workbench section of the menu "Run -> Edit Configuration"
* To add framework and other dependencies to be build, click the + button under the "Before launch" section, click "Run External tool", click the + button again, name this tool as "Build all", in the Program option select your copy of ninja, an example path is: "{MAMBAFORGE_INSTALL_DIR}/envs/mantid-developer/bin/ninja", then ensure that the working directory is your mantid build directory so that it builds the default ninja/CMake target.

Debugging Tests with CLion
--------------------------

* To debug tests you need to select the executable those tests exist inside of, for example an Algorithm test is in the AlgorithmTest executable/CMake test target
* Edit this configuration by going to Run -> Edit Configurations...
* In this menu add the test name that you are trying to debug to the Program arguments option, in the AlgorithmTest example we can add CalculateZscoreTest and it will run the CalculateZscoreTest.

VSCode
######
