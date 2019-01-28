.. _UsingPycharmWithWorkbench:

============================
Using PyCharm with Workbench
============================

Developing and debugging with PyCharm on Windows
################################################
The first thing that needs to be done is creating the PyCharm project and configuring the project settings. Please follow the instructions at :ref:`setting-up-pycharm-on-windows`.

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

Running other Mantid widgets in PyCharm
---------------------------------------
The widgets in ``mantidqt/widgets`` can be run independently of the Workbench. They still need the relevant PATHs available - to load the ``mantid``, ``mantidqt``, and various other libraries.

The easiest way to do this is to set the script path to the widget's startup script, but leave the working directory to point at ``<Mantid Build Directory>/bin/Debug`` or ``<Mantid Build Directory>/bin/Release``

For example to run the MatrixWorkspaceDisplay:

1. Copy your default configuration (the copy icon on the top left of the configuration window).
2. Set the Script Path to ``<Mantid Source Dir>/qt/python/mantidqt/widgets/matrixworkspaceviewer``. This package has a ``__main__.py`` file which makes it runnable.
3. You might have to CHANGE back the working directory to ``<Mantid Build Dir>/bin/Debug``.
4. Click OK, then running the configration should start the MatrixWorkspaceDisplay.
