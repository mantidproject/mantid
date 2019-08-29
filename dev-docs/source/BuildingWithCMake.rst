.. _BuildingWithCMake:

===================
Building with CMake
===================

.. contents::
  :local:

CMake is the build system for the entirety of Mantid (Framework, MantidQt & MantidPlot). It is used to generate native build files for your platform, which can be Makefiles (for use with make, nmake or jom) for command line builds or project/solution files for an IDE (e.g. Visual Studio, Eclipse, Qt Creator, XCode).

Environment
###########

The  :ref:`GettingStarted` page describes how to set up your environment to build Mantid. Follow those instructions and install the Mantid dependencies first.

Also, if you use the Ninja generator then the executable is called ``ninja-build``.

CCache
######

Mantid's cmake is configure to use the `ccache <https://ccache.samba.org/>`_ tool if it is available. It is highly recommended that this be used on Linux/macOS systems.

For Linux either run either

.. code-block:: sh

  sudo yum install ccache

on Red Hat, or

.. code-block:: sh

  sudo apt-get install ccache

on Ubuntu.

For macOS run:

.. code-block:: sh

  brew install ccache

After it is installed run ``ccache --max-size=20G`` to increase the size of the cache.

If you're build with ``ccache`` exhibits warnings that are not usually present then try setting the ``ccache --set-config=run_second_cpp="true"`` config option (or set ``CCACHE_CPP2=yes`` environment variable on older versions).

Network Drives
--------------

The default location for the cache directory is ``$HOME/.ccache``. If you're home directory is on a network-mounted drive then the location of this cache be moved to provide the best performance. On newer versions of ``ccache`` run ``ccache --set-config=cache_dir=PATH_TO_CACHE``. Older versions (<3.2) do not allow this and must fall back to setting the ``CCACHE_DIR`` environment variable in your shell profile.

Configuring your build
######################

CMake encourages the use of 'out of source' builds. This means that all generated files are placed in a separate directory structure to the source files. This separation makes a full clean easier (you just delete everything) and means that you can have different types of build (Release, Debug, different compiler versions, ....) in separate places (N.B. For Visual Studio & XCode, you can still select the type of build from within the IDE).

From the command line
---------------------

* If wanting an out of source build, create the directory you want to build in and ``cd`` into it.
* On Windows, you may need to be in a Visual Studio Command Prompt.
* Run ``cmake /path/to/Mantid``, or to ``/path/to/Mantid/Framework`` if you only want a build of the Framework (typically not recommended, but possible nonetheless). This will generate build files using the default generator for your platform (e.g. Unix Makefiles on Linux).
* If you want to use a specific generator (run ``cmake --help`` for a list of available generators for your platform), use the ``-G`` option, e.g. ``cmake -G"NMake Makefiles" /path/to/Mantid``.
* If you want to set the build type (e.g. Release, Debug) you can run cmake with the ``-i`` option or by passing the argument ``-DCMAKE_BUILD_TYPE=Debug`` to cmake. The default is Release.
* Please note that the executable is called ``cmake3`` on Red Hat 7 / CentOS7.
* On Red Hat 7 / CentOS7 mantid uses `devtoolset-7 <https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/>`_. This means that you need to wrap your initial ``cmake`` command as ``scl enable devtoolset-7 "cmake3 /path/to/source"``

From the CMake gui
------------------

* The cmake gui is available from, e.g., the Windows Program menu or the command line executable ``cmake-gui``.
* Start it and click the "Browse Source" button to point to ``/path/to/Mantid``.
* Click "Browse Build" and point to the directory you want to build into - it's recommended that you create a new directory for this (see above), though it can be the same as the source directory.
* Click "Configure" down near the bottom of the window.
* A new window will appear asking which 'Generator' you want to use:

  * Linux/Mac developers should choose ``Ninja``
  * Windows developers should choose ``Visual Studio 16 2019`` and in the _Optional platform for generator\_ box select ``x64``. If you see errors related to HDF5 then you have most likely selected the wrong platform.

* Wait a while....
* You will be presented with a list of options in red that can in principle be changed. You probably don't want to change anything, except perhaps checking ``MAKE_VATES`` if you want to build that.
* Click "Configure" again and wait....
* Finally, click "Generate". This will create the build files, e.g. for a Visual Studio build there will be a ``Mantid.sln`` in the directory you selected as your build directory.

Data Files Location
-------------------

Mantid used the CMake ExternalData system for managing testing data. See :ref:`DataFilesForTesting` for further instructions.

With Qt Creator
---------------

`Qt Creator <http://qt.nokia.com/products/developer-tools/>`_ has some really nice features (it's cross-platform, you can directly open Qt Designer within it, you can highlight a Qt type and go directly to it's help page, it knows about Qt types when debugging....).
The nice feature in this context is that it has CMake support built in. So you can just open the project by pointing to the main CMakeLists file and then run CMake all within the IDE itself.

Building and working with CMake
###############################

Building from IDE:

* Windows using Visual studio: Use the ``visual-studio.bat`` generated in the build directory to start the IDE. This sets up the environment correctly.
* Otherwise start your IDE and point to or import the generated solution/project files

Command line: run ``make``, ``nmake`` or ``jom`` to build the whole of Mantid (sub-targets are available - run ``make help`` to see them).

Working with CMake:

* You should typically never have to run CMake manually again (unless you want to create a new, separate build) - it will be run automatically if one of the CMake input files changes.
* It should be rare that you will need to edit the CMake build (``CMakeLists.txt``) files. The most common occurrence will be when you add a new file. This must be added to the corresponding CMakeLists file, e.g. if you add a file to Kernel, edit ``Mantid/Framework/Kernel/CMakeLists.txt`` to add the source, header and test files to the long lists of filepaths at the top of the file.
* The class maker utility (:ref:`ToolsOverview`) can edit the ``CMakeList.txt`` for you automatically
* There are similar places in the Qt projects for ui files and files that need moc-ing.
* If you add a new dependency, that will need to be added (this is less straightforward - do ask for help).
* Cache variables can be added via the CMake Gui or by running ``ccmake``.

Building the installer package
##############################

* For Windows only, you first need to install NSIS, available at: http://nsis.sourceforge.net/Download. Ensure that the install directory is added to the PATH. You should be able to type ``makensis /?`` in a command prompt.
* Run CMake with "ENABLE_CPACK" enabled. If using the GUI you need to click the "Advanced" checkbox to see this option.
* You will now have a build target called `package` available to create the installer package.

Caveats and Known Issues
########################

* For Visual Studio & XCode, the libraries and executable are put into ``Mantid/bin/Release``, ``Debug``, etc.
* There is a known issue with using source control with Eclipse on an out of source build. Set the cache variable ``ECLIPSE_CDT4_GENERATE_SOURCE_PROJECT`` to true and CMake will generate a set of 'dummy' project files within the source tree so that you can import that project and use it for source control actions.

Tips
####

* Running unit test executables directly with the CMake-generated ``Mantid.properties`` file will lead to a bunch of logging output to the console. You are encouraged to use CTest instead, which suppresses this output automatically. Otherwise, adding the line ``logging.channels.consoleChannel.class = NullChannel`` to your Mantid.user.properties file will turn if off.
* If you have more than one gcc and want to build with a version other than the default (e.g. on RedHat), setting CC & CXX environment variables is one way to make it so.
