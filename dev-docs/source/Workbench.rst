.. _Workbench:

=====================
Workbench Development
=====================

.. contents::
  :local:


Overview
########

The workbench is the new PyQt-based GUI that will be the primary interface for
interacting with the mantid framework. The plotting is provided by
`matplotlib <https://matplotlib.org/>`_. It will eventually replace MantidPlot.

Building
########

The following build instructions assume you have followed the instructions on the :ref:`GettingStarted` pages and can build mantid and MantidPlot. To enable the
build of the workbench simply set the cmake flag ``ENABLE_WORKBENCH=ON`` and
build as normal. A ``workbench`` startup script (Linux/macOS) or executable (Windows) will appear in the ``bin`` folder. For Windows the executable will appear in the configuration subdirectory of ``bin``.

Packaging
#########

Packaging is currently only supported on Linux platforms and must be enabled
using the cmake flag ``PACKAGE_WORKBENCH=ON``.

Developing and debugging with PyCharm on Windows
################################################
The first thing that needs to be done is creating the PyCharm project and configuring the project settings. Please follow the instructions at :ref:`GettingStartedWithPyCharm`.

After the project settings have been configured, a Run/Debug configuration needs to be created. To edit the configurations go to ``Run->Run...`` and select ``Edit Configurations``. Select ``Templates->Python``, and hit the green ``+`` in the top left. 
The necessary changes to the configuration are:

- Select the Mantid Python interpreter that was added to the project in the instructions above. Do not use the default interpreter on Windows.
- Set up *Script Path* and *Working Directory* as follows:

Paths for running a Debug build:
    - Set the *Script Path* to ``<Mantid Build Directory>/bin/Debug/workbench-script.pyw``
    - Set the *Working Directory* to ``<Mantid Build Directory>/bin/Debug``

Paths for running a Release build:
    - Set the *Script Path* to ``<Mantid Build Directory>/bin/Release/workbench-script.pyw``
    - Set the *Working Directory* to ``<Mantid Build Directory>/bin/Release``

Note that the only difference here is the change from ``/bin/Debug/`` to ``/bin/Release/``. 

Make sure you have finished the build you are using (Debug or Release), or there will be import errors.

Common errors
-------------

    qtpy.PythonQtError: No Qt bindings could be found

``<Mantid Source Directory>/external/src/ThirdParty/lib/qt5/bin`` is missing from the ``Path`` environment variable.

    ImportError: DLL load failed: The specified module could not be found.

``<Mantid Source Directory>/external/src/ThirdParty/lib/qt5/lib`` is missing from the ``Path`` environment variable.

The fix for these errors is to make sure you have **started** PyCharm through the ``<Mantid Build Directory>/pycharm.bat`` script. This sets up the ``PATH`` variable for the Python imports.

Additionally, check that your PyCharm Run/Debug configuration *does not* overwrite the ``PATH`` variable. 
To check go to ``Edit Configurations -> Environment Variables`` and click the folder icon on the right side. In the ``Name`` column there should not be a ``Path`` variable.
If there is one, try deleting it and running your configuration again.

Identifying issues with PyCharm configuration
---------------------------------------------
Follow these steps to narrow down the root of potential errors:

- Go to the Mantid Build Directory
- Start ``command_prompt.bat``. If the ``command_prompt.bat`` file is missing, the build has not been fully generated from `CMake` or is corrupted.
- In the command prompt open the Python interpreter with ``python``.
- Try to import the package where the error is happening. For example if there is an error on ``import qtpy``, ``from PyQt5 import QtCore``, try running that line in the interpreter.
- If the import succeeds, then there is a problem with the ``PATH`` configuration in PyCharm.
- If the import fails, then it is possible that Mantid has not been fully built. If you are trying to import ``PyQt4``/``PyQt5``/``qtpy``, and it fails to import from the command prompt, then the ``external`` dependencies might not be downloaded or are corrupted.
