
.. _OSXCondaPackaging:

======================
OSX Conda Packaging
======================

.. contents::
  :local:


To package a C++ project you first need to create a standalone install.
CMake helps with this by providing a couple of utilities to copy library dependencies into your install.

The first is ``GET_RUNTIME_DEPENDENCIES``, which can be supplied with a list of libraries and executables which
will be analysed for their dependencies:

.. code-block:: cmake

    file(GET_RUNTIME_DEPENDENCIES
    [RESOLVED_DEPENDENCIES_VAR <deps_var>]
    [UNRESOLVED_DEPENDENCIES_VAR <unresolved_deps_var>]
    [CONFLICTING_DEPENDENCIES_PREFIX <conflicting_deps_prefix>]
    [EXECUTABLES [<executable_files>...]]
    [LIBRARIES [<library_files>...]]
    [MODULES [<module_files>...]]
  )

Mre recently (CMake 3.21) an optional argument to the install directive install a ``RUNTIME_DEPENDENCY_SET`` along with the target.

.. code-block:: cmake

    install(TARGETS targets...
        [RUNTIME_DEPENDENCIES args...|RUNTIME_DEPENDENCY_SET <set-name>]
        )

For the time being, we've gone with ``GET_RUNTIME_DEPENDENCIES`` as we have not moved to CMake 3.21 yet. The dependency bundling is performed in ``BundleOSXConda.cmake``
In the future we could add the runtime dependency install to each target, through the ``mtd_install_targets`` CMake macro.

Once the C++ dependencies are resolved, we need to consider how to include python in the package.
Our design here was influenced by Juypter which have recently started shipping a desktop application. Their application shares some similarities with mantid,

- Contains a GUI based application
- Nested inside the GUI application sits a python instance

To solve the issue of satisfying the python dependency, Juypter used the conda constructor tool.
This tool creates (.sh, pkg, exe) packages which can be used to install a conda environment specified by a .yaml file, e.g:

.. code-block:: yaml

    name: MantidPython
    version: X
    installer_type: all

    channels:
    - conda-forge

    specs:
    - python
    - numpy
    - ...

which creates a package named ``MantidPython`` that installs a python environment and all of its dependencies.

Jupyter bundles a conda constructor installer (in the form a shell script) into their application bundle.
While installing ``Juypterlab`` application this shell script is executed, placing a full python package into their application.
The GUI application can then launch and use this python environment.


====================
Build instructions
====================

.. figure::  images/packagingdigramOSX.pdf

Create the conda environment

    .. code-block:: sh

        conda env create -f ./mantid-developer-osx.yml
        conda activate mantid-developer

Setup the build

    .. code-block:: sh

        mkdir build
        cd build
        cmake .. --preset=osx -DENABLE_CPACK=ON -DCMAKE_INSTALL_PREFIX=../../my_install_folder/mantid -DCMAKE_BUILD_TYPE=Release

Run the build

    .. code-block:: sh

        ninja
        cmake --install . --component Runtime

Run CPack

     .. code-block:: sh

        cpack -C Release




====================
The .pkg installer
====================

The above steps creates a .pkg installer in the build folder. The pkg contains a conda constructor installer for a python environment. This environment would contain all the python dependencies required by mantid. It is automatially installed with a postinstall script during the package installation. This script contains the following:

.. code-block:: sh

    #!/bin/bash
    sh /Applications/MantidWorkbench.app/Contents/Resources/env_installer/MantidPython-0.1.0-MacOSX-x86_64.sh -b -u -p /Applications/MantidWorkbench.app/Contents/Resources/mantid_python
    exit 0

::

    Once the pkg is executed it installs a package in Applications with the following layout:
    MantidWorkbench
    │
    └───Contents
    │   │
    │   └───MacOS
    │   │     │- MantidWorkbench
    │   │     │- libMantidAPI.dylib
    │   │
    │   └───Framework
    │   │     │- libQt5Gui.5.dylib
    │   │     │- llibboost_filesystem.dylib
    │   │
    │   └───Resources
    │         │- env_installer/mantidpython.sh
    │         │- mantid_python/


The `mantid_python` folder contains our full python environment created using the postinstall script and the installer from conda constructor. It looks like:

::

    mantid_python
    │
    └───lib
    │   │- python3.8
    │
    └───bin
        │- python

THe contents of the python environment is controlled by the construct.yaml file in the mantid source tree.
