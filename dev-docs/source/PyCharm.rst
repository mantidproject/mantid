.. _pycharm-ref:

=======
PyCharm
=======

.. contents::
  :local:

Selecting PyCharm versions
##########################

There are two versions of PyCharm that are in use in the team, Community and Professional. The main difference for our workflow is that professional offers support for remote debugging which can help with debugging Python code that is called from C++, such as algorithms called from Python interfaces.

If you haven't installed PyCharm yet do that now, PyCharm can be installed from `here <https://jetbrains.com/pycharm/download/>`_.

Setup Python development environment with Conda
###############################################

The assumption has been made that you have setup and built Mantid already, if you have not, please do so before hand by following, `this guide <GettingStarted/GettingStarted.html>`_.

At any point in these instructions where ``DebugWithRelRuntime`` is used (including in file paths),
you can replace it with any other build type such as ``Debug`` or ``Release``.
We use ``DebugWithRelRuntime`` for Conda specific builds to allow debugging due to ``Debug`` not being functional with the ``Release ABIs``.

- Open PyCharm
- If no project has been selected already, open the Mantid source code as a project.
- Open the ``File->Settings menu``.
- In the left hand side select the option that is ``Project: {SOURCE_CODE_FOLDER_NAME}`` for example ``Project: mantid``.
- Select the ``Python Interpreter`` option.
- Click ``Add Interpreter`` on the top right, then ``Add Local Interpreter...``.
- From the left side of the window select ``Conda Environment``.
- Add the path to your Conda executable, e.g. ``C:\Users\<username>\AppData\Local\mambaforge\Scripts\conda.exe`` and click ``Load Environments``.
- Click the ``Use Existing environment`` radio button and select the ``mantid-developer`` environment in the drop down list.
- Click OK to close the window.
- Ensure that next to ``Python Interpreter:`` it says ``mantid-developer``. You will also see a list of the python packages installed in your mantid-developer environment.
- Then click Apply.
- Back on the left side, under ``Python Interpreter`` there should be an option for ``Project Structure``. Select that.
- If you do not build Mantid in the same directory as your source but somewhere else add this as another Content Root, by selecting ``+ Add Content Root`` on the right hand side now.
- In the file tree, select each of the following and mark them as source directories by clicking the ``Sources`` button while they are selected:

    - ``{SOURCE}/scripts``
    - ``{SOURCE}/Framework/PythonInterface``
    - ``{SOURCE}/qt/applications/workbench``
    - ``{SOURCE}/qt/widgets``
    - ``{SOURCE}/qt/python/mantidqt``
    - ``{SOURCE}/qt/python/mantidqtinterfaces``

- In the file tree select your build directory and mark as excluded by clicking the ``Excluded`` button whilst it's selected.
- On Windows, select the ``{BUILD}/bin/DebugWithRelRuntime`` directory and mark as source by clicking the ``Sources`` button whilst it's selected.
- On Linux or MacOS, select your ``{BUILD}/bin`` directory and mark as source by clicking the ``Sources`` button whilst it's selected.
- Click Apply, and then OK to close the window.

.. _debug-workbench-in-pycharm-ref:

Debug Python in Workbench
#########################

Now that your Python development environment has been setup we can setup the debugging using Workbench.

- With an open project in Pycharm, open the Play configuration menu by Opening ``Run->Edit Configurations...``.
- Click the ``+`` icon top left
- Select Python
- Name it something to do with ``Workbench``
- In the ``Script Path:`` box, on Linux/MacOS enter the ``{BUILD}/bin/workbench`` Python script, on Windows enter ``{BUILD}/bin/DebugWithRelRuntime/workbench-script.pyw``, ``.pyw`` files will not appear in the search window as it only shows ``.py`` files, so you cannot search for it with the GUI.
- In the ``Parameters`` box add ``--single-process`` so that the multiprocess startup is disabled and breakpoints can be attached to the primary process. See the :ref:`Running Workbench <RunningWorkbench>` documentation for more information.
- In the ``Working directory:`` box, on Linux/MacOS enter the ``{BUILD}/bin`` directory, on Windows enter ``{BUILD}/bin/DebugWithRelRuntime`` directory.
- Ensure the ``Python Interpreter:`` box is set to use your ``(mantid-developer)`` Conda environment.
- Click OK to save and exit the window.
- You can now click the green play button in the top right of the window to create a Workbench instance from pycharm.
- Alternatively you can click the green bug next to the green play button to start a debug session.

.. note::
  On Windows, you may see the following error when launching workbench in debug mode:

  .. code-block:: bash

    incompatible copy of pydevd already imported:
    C:\Program Files\JetBrains\PyCharm Community Edition 2023.1.2\plugins\python-ce\helpers\pydev\_pydev_bundle\__init__.py
    C:\Program Files\JetBrains\PyCharm Community Edition 2023.1.2\plugins\python-ce\helpers\pydev\_pydev_bundle\_pydev_calltip_util.py
    ...

  To resolve the error, remove **only** the debugpy package from your Conda environment with

  .. code-block:: bash

    conda remove debugpy --force

  The ``--force`` argument tells Conda to remove the single package only, ignoring the packages that depend on debugpy.
  Note that the Mamba implementation of ``remove --force`` did not skip dependency checking until version 1.5.2 (October 2023).
  If your version of Mamba is older than this, use the Conda command.

.. include:: ./macos-opengl-version-warning.txt

Debug Python in unit tests
##########################

This section assumes you have followed all previous instructions for debugging Python in workbench

There are 2 main ways to debug Python unit tests using the Unittests Python module.

1. Navigate to the test you want to run in PyCharm, on the left side of the file in the margin, just to the right of the line numbers there should be a green play button, click that and it will let you start a debug or normal run of the tests.

2. A little more involved, but is easier to expand and test many things at once.
    - Same as when creating the Workbench debug session. Open the configuration menu by navigating to ``Run->Edit Configurations...``
    - Click the ``+`` icon top left
    - Select Unittests
    - Give an appropriate name for the section of code you will be testing
    - You have 3 options, enter the module name, script path or custom.

        - Module name for testing workbench project recovery tests looks like this ``workbench.projectrecovery``, this runs all of the tests in the project recovery section. This is very useful for testing all of a specific section of the code base, without running it in a terminal.
        - Script path is very similar instead of passing a module name, you just give a filepath such as ``{SOURCE}/qt/applications/workbench/workbench/projectrecovery`` this achieves exactly the same as the previous step.
        - Custom is for passing custom arguments to the Unittests executable such as these: https://docs.python.org/3/library/unittest.html#command-line-interface

Debug Python in system tests/remote debugging
#############################################

This functionality is useful for debugging Python code that is spawned in separate threads, such as Python algorithms called from C++ and system tests.

A PyCharm Professional license is required to use the Remote Debugging feature.

This section assumes you have followed all previous instructions for debugging Python in workbench and unit tests.

- Like the Unit tests and workbench we need to add it as a configuration, open the configuration menu by navigating to ``Run->Edit Configurations...``
- Click the ``+`` icon top left
- Select ``Python Debug Server``
- Give an appropriate name for remote debugging such as ``Remote Debugging``
- Copy the snippet of code that consists of ``pip install pydevd-pycharm``
- Close the configuration window
- Open Terminal at the bottom of the PyCharm window
- Paste the snippet of code and hit enter, this will install the remote debugger for PyCharm to use.
- Once installed, re-open the configuration menu by navigating to ``Run->Edit Configurations...``
- Ensure that the previously created ``Python Debug Server`` is selected in the left hand side tree selection.
- Ensure that you set the port box to something that isn't 0 and isn't in use by your system at present such as ``8080``.
- Copy the snippet of Python code that looks like this:

    .. code-block:: python

        import pydevd_pycharm
        pydevd_pycharm.settrace('localhost', port=8080, stdoutToServer=True, stderrToServer=True)

- Paste this code where you want to start debugging from, this will act like a breakpoint during normal debugging.
- Click the drop down menu next to the play icon in the top right. Select the ``Python Debug Server`` you configured, then click the debug next to the play icon.
- Run the python code that you want to debug, for example run the system tests, and it will pause execution on where you pasted your remote debug code earlier.
- Any new breakpoints can be added like normal but they must come after the remote code snippet pasted earlier.

==================================================================================
Legacy and not maintained past this point (Only use if explicitly not using Conda)
==================================================================================

Setting up PyCharm on Windows
#############################

1. Once PyCharm is open, set up the project. Go to ``File->Open`` and select the root directory in which both your source and build directories reside.

   Go to ``File->Settings``, then under ``Project`` you will set two sub-menus ``Project Interpreter`` and ``Project Structure``. The interpreter defines the Python executable that will be used to run your code, and the structure menu allows you to decide which folders within the project to include and index.

2. In the ``Project Interpreter`` sub menu, at the top select the options button and click ``Add...``, a new window should appear titled "Add Python Interpreter". In the menu on the left, if you are using Conda select "Conda Environment", if you haven't set up Conda follow the Getting Started guidance for it, select existing environment and if not present already put in the path to your Python interpreter, and your conda executable. Alternatively select "System Interpreter" (a version of Python with all the correct variables set already exists within Mantid, if you are not using Conda). Click on the ``...`` to open a file browser, and navigate to;

   .. code-block:: sh

      <Mantid Source Directory>/external/src/ThirdParty/lib/Python3.8/Python.exe

   This is the interpreter, so select "Ok" and apply the changes. This should bring up a list of all the packages associated to the interpreter. There should be many packages, however you should not see PyQt (but instead QtPy).

3. In the ``Project Structure`` sub menu you should see your root directory with the source/build directories both visible (if not, add them). The folder structure should be present in the centre of the window allowing you to mark folders orange (excluded) or blue (source). Source directories will be searched for Python code.

   Within the source directory add the following to your sources:

   .. code-block:: sh

       <Mantid Source Directory>/scripts
       <Mantid Source Directory>/Framework/PythonInterface
       <Mantid Source Directory>/qt/applications/workbench
       <Mantid Source Directory>/qt/widgets
       <Mantid Source Directory>/qt/Python
       <Mantid Source Directory>/external/src/ThirdParty/lib


   If you are writing scripts in any other directories, you can also mark them as sources. This helps PyCharm give better auto-complete and import suggestions during development.

   Additionally, in the Mantid build directory add the following as source folders:

   .. code-block:: sh

       <Mantid Build Directory>/bin/Debug

   here we are setting up PyCharm for the Debug build, you would use ``/bin/Release`` instead if you are building mantid in release mode.

4. The environment needs to be set up before running the configuration. Follow the instructions below to use either the EnvFile plugin (recommended) or manual path setup.

NOTE : In some cases, imports in the code will still be highlighted red when they come from folders within the ``script/`` folder, or from other folders entirely. To fix this simply add the relevant folder that contains the module you are importing in the same fashion as step 3 above.

.. _pycharm-debugging-env-file:

Running Files in the Debugger with EnvFile extension
####################################################

Do not run files in the debugger with EnvFile extension with Conda, as Conda does this job for you.

Running Python code from within PyCharm which depends on the Python API, or PyQt for example requires one extra step. Because the source root labelling from the previous section only affects PyCharm searching and not the run configuration, before running the file we must set up the run configuration correctly.

4. Install the EnvFile plugin by Borys Pierov. The plugin can be installed in multiple ways:

   a) Open Settings(CTRL + SHIFT + S), to go Plugins and search for ``EnvFile``. Install and restart PyCharm.
   b) Go to the plugin's `webpage <https://plugins.jetbrains.com/plugin/7861-envfile>`_, download and install it.

5. To edit the configurations go to Run->Run... and select Edit Configurations. Notice that there is now a ``EnvFile`` tab under the configuration's name.
   - Note that you have to do that for each configuration, or you can change the template configuration, and all configuration that use that template will have the EnvFile setup.
6. Open the ``EnvFile`` tab, check ``Enable EnvFile`` and ``Substitute Environmental Variables (...)`` - this allows setting up the third-party paths dynamically.
7. Click the ``+`` (plus) on the right side, select the ``pycharm.env`` file in the root of the **build** directory.

For running the Workbench continue onto :ref:`Workbench`, and follow the instructions to set up the *Script Path* and *Working Directory*.

Advantages of this approach:

- You can have multiple instances of PyCharm running with environment configuration for separate repositories. This is otherwise not possible, as all PyCharm instances seem to share a parent process and environment. (as is the case of 11/01/2019, it might change in the future)
- This makes possible switching projects for multiple repositories via the File > Open Recent ... menu, as when the new project is opened its environment won't be poluted with environment variables from the last one.

  - This can cause errors when the external dependencies aren't quite the same between all the repositories, as some packages might be missing, or be different versions.

Disadvantages:

- Additional setup for each configuration necessary. Thankfully, if the template is edited to have the correct ``EnvFile`` setup, all copies of it will have it too. Copying an already existing configuration also copies the ``EnvFile`` setup.


Running Files in the Debugger without EnvFile extension
#######################################################

This can be done in two ways:

- Open PyCharm using ``pycharm.bat`` which can be found in the build directory (this sets some additional environment variables compared with simply opening PyCharm directly).

  - This is preferred if you only have 1 repository with which PyCharm is used. If you need to use PyCharm on multiple repositories, it is recommended that you use the EnvFile extension.

- To edit the configurations go to ``Run->Run...`` and select ``Edit Configurations``. This should open up a sub window. Hit the green ``+`` in the top left to create a new configuration and name it. In order to tell PyCharm where to look for Python modules and libraries we need to add some folders to the ``PATH`` environment variable. Click on the ``...`` next to the *Environment Variables* box, and hit the ``+`` icon. In the Name column enter "PATH", in the value column enter the following;

   .. code-block:: sh

       <Mantid Build Directory>\bin\Debug;
       <Mantid Source Directory>\external\src\ThirdParty\bin;
       <Mantid Source Directory>\external\src\ThirdParty\bin\mingw;
       <Mantid Source Directory>\external\src\ThirdParty\lib\Python3.8;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\plugins;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\bin;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\lib;
       %PATH%

The semi-colon delimited list of paths should end in ``;%PATH%`` so that we prepend to the existing list of paths rather than overwriting them.

You should now be able to run and debug the scripts using the newly created configuration, by adding the full path of the file in the ``Script path`` box at the top of the configuration window.

As an example, create a new file in ``<Mantid Source Directory>/scripts/`` called ``test.py``. Copy into it the Python code below.

Testing using PyQt
##################

To test that the above instructions have worked, you can simply create a new Python file with the following content (for PyQt5)

.. code:: python

    # Check that PyQt imports
    from qtpy import QtCore, QtGui, QtWidgets
    # Check that the Mantid Python API imports
    import mantid.simpleapi

    class DummyView(QtWidgets.QWidget):

        def __init__(self, name, parent=None):
            super(DummyView, self).__init__(parent)
            self.grid = QtWidgets.QGridLayout(self)
            btn = QtWidgets.QPushButton(name, self)
            self.grid.addWidget(btn)

    if __name__ == "__main__":
        import sys
        app = QtWidgets.QApplication(sys.argv)
        ui = DummyView("Hello")
        ui.show()
        sys.exit(app.exec_())


Local Debugging of Unit Tests with PyCharm
##########################################

This **does not** require a PyCharm Professional license for debugging, but requires additional setup for running unit tests.

1. Go to your Run/Debug Configurations.
2. Open Templates > Python tests > Unittests configuration.
3. Set the working directory to ``<Mantid Build Dir>/bin/Debug``, for a Debug build, or ``<Mantid Build Dir>/bin/Release`` for a Release build.
4. Add the EnvFile to the Unittests configuration, instructions in :ref:`pycharm-debugging-env-file`.
5. You should now be able to click the Run/Debug icons next to each unit test method or class to run/debug them.

Setting up PyCharm on Linux
###########################

1. Use the native Python interpreter (``/usr/bin/Python3``) rather than from ``<Mantid Source Directory>/external/src/ThirdParty/lib/Python3.8/Python.exe``
2. In the ``Project Structure`` sub menu you should see your root directory with the source/build directories both visible (if not, add them). The folder structure should be present in the centre of the window allowing you to mark folders orange (excluded) or blue (source). Source directories will be searched for Python code.

   Within the source directory add the following to your sources:

   .. code-block:: sh

       <Mantid Source Directory>/scripts
       <Mantid Source Directory>/Framework/PythonInterface
       <Mantid Source Directory>/qt/applications/workbench
       <Mantid Source Directory>/qt/widgets
       <Mantid Source Directory>/qt/Python


   If you are writing scripts in any other directories, you can also mark them as sources. This helps PyCharm give better auto-complete and import suggestions during development.

   Additionally, in the Mantid build directory add the following as source folders:

   .. code-block:: sh

       <Mantid Build Directory>/bin/

   It is recommended that you add the whole build folder to ``excluded``. This will not interfere with the ``bin`` directory, inside the build, being used as a source folder. It will just limit the scope that PyCharm searches for files, classes, etc.

3. Go to Run->Run... and select Edit Configurations. Go to Templates > Python. Make ``<Mantid Build Directory>/bin;`` the ``Working Directory``. This will then be used for all Python configurations you make.


Useful Plugins
##############

You can install non-default plugins by pressing ``Ctrl+Alt+S`` to open the **Settings/Preferences** dialog and then going to **Plugins**.
From here you can manage plugins, or add new ones by clicking **Browse repositories**.

The following non-default plugins are things our team has found useful for Mantid development:

- **Markdown support** - Side by side rendering of markdown documents such as``.md`` , ``.rst`` (requires `Graphviz <https://graphviz.gitlab.io/download/>`_ to show graphs in preview)
- **dotplugin** -  Syntax highlighting for ``DOT``
- **BashSupport** - Syntax highlighting for ``BASH`` scripts
- **CMD Support** - Syntax highlighting for ``.BAT`` ~scripts

Please add to this list if you find a useful plugin of your own

Remote Development
##################

Note: Requires PyCharm Professional.

PyCharm supports deployment and syncronisation of written code to a remote server via SSH.

Open a local copy of the project and then follow the guides here for `configuring the remote interpreter <https://www.jetbrains.com/help/pycharm/configuring-remote-interpreters-via-ssh.html>`_ and `creating a deployment configuration <https://www.jetbrains.com/help/pycharm/creating-a-remote-server-configuration.html>`_.
