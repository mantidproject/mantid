.. _GettingStartedWithPyCharm:

Getting Started with PyCharm
============================

PyCharm can be installed from `here <https://jetbrains.com/pycharm/download/>`_.

This tutorial assumes you are familiar with the process of building Mantid (with separate source and build directories inside a root directory), and that you have built a working version. If you are unclear about this see :ref:`here <GettingStarted>`.

.. contents::
  :local:

.. _setting-up-pycharm-on-windows:

Setting up PyCharm on Windows
#############################

1. Once PyCharm is open, set up the project. Go to ``File->Open`` and select the root directory in which both your source and build directories reside.

   Go to ``File->Settings``, then under ``Project`` you will set two sub-menus ``Project Interpreter`` and ``Project Structure``. The interpreter defines the python executable that will be used to run your code, and the structure menu allows you to decide which folders within the project to include and index.

2. In the ``Project Interpreter`` sub menu, at the top select the options button and click ``Add...``, a new window should appear titled "Add Python Interpreter". In the menu on the left, select "System Interpreter" (a version of Python with all the correct variables set already exists within Mantid). Click on the ``...`` to open a file browser, and navigate to;

   .. code-block:: sh

      <Mantid Source Directory>/external/src/ThirdParty/lib/python2.7/python.exe

   This is the interpreter, so select "Ok" and apply the changes. This should bring up a list of all the packages associated to the interpreter. There should be many packages, however you should not see PyQt (but instead QtPy).

3. In the ``Project Structure`` sub menu you should see your root directory with the source/build directories both visible (if not, add them). The folder structure should be present in the centre of the window allowing you to mark folders orange (excluded) or blue (source). Source directories will be searched for python code.

   Within the source directory add the following to your sources:

   .. code-block:: sh

       <Mantid Source Directory>/Framework/PythonInterface
       <Mantid Source Directory>/scripts
       <Mantid Source Directory>/qt/applications/workbench
       <Mantid Source Directory>/qt/widgets
       <Mantid Source Directory>/qt/python
       <Mantid Source Directory>/external/src/ThirdParty/lib


   The first folder can be replaced with the folder that contains your code, if you aren't writing code in ``scripts/``.

   Additionally, in the Mantid build directory add the following as source folders:

   .. code-block:: sh

       <Mantid Build Directory>/bin/Debug

   here we are setting up PyCharm for the Debug build, you would use ``/bin/Release`` instead if you are building mantid in release mode.

4. The environment needs to be set up before running the configuration. Follow the instructions below to use either the EnvFile plugin (recommended) or manual path setup.

NOTE : In some cases, imports in the code will still be highlighted red when they come from folders within the ``script/`` folder, or from other folders entirely. To fix this simply add the relevant folder that contains the module you are importing in the same fashion as step 3 above.

.. _running-file-debug-with-envfile-extension:

Running Files in the Debugger with EnvFile extension
####################################################

Running python code from within PyCharm which depends on the python API, or PyQt for example requires one extra step. Because the source root labelling from the previous section only affects PyCharm searching and not the run configuration, before running the file we must set up the run configuration correctly.

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

- To edit the configurations go to ``Run->Run...`` and select ``Edit Configurations``. This should open up a sub window. Hit the green ``+`` in the top left to create a new configuration and name it. In order to tell PyCharm where to look for python modules and libraries we need to add some folders to the ``PATH`` environment variable. Click on the ``...`` next to the *Environment Variables* box, and hit the ``+`` icon. In the Name column enter "PATH", in the value column enter the following;

   .. code-block:: sh

       <Mantid Build Directory>\bin\Debug;
       <Mantid Source Directory>\external\src\ThirdParty\bin;
       <Mantid Source Directory>\external\src\ThirdParty\bin\mingw;
       <Mantid Source Directory>\external\src\ThirdParty\lib\python2.7;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\plugins;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt4\bin;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\bin;
       <Mantid Source Directory>\external\src\ThirdParty\lib\qt4\lib;
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
    # Check that the Mantid python API imports
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
4. Add the EnvFile to the Unittests configuration, instructions in :ref:`running-file-debug-with-envfile-extension`.
5. You should now be able to click the Run/Debug icons next to each unit test method or class to run/debug them.


Remote Debugging of Unit Tests with PyCharm
###########################################

This requires a PyCharm Professional license for the Remote Debugging feature.

This approach can be used to debug unit tests. However, as the required package ``pydevd`` is not shipped with Mantid, we need to manually add it at runtime. This can be done by appending a directory that contains the installed ``pydevd`` package on the ``PYTHONPATH``. The following code does so at runtime::

    PYTHON_ROOT="<Change this to point to a Python installation that has pydevd installed>"
    # PYTHON_ROOT="c:/users/qbr77747/apps/miniconda3"
    import os
    import sys
    sys.path.append(os.path.join(PYTHON_ROOT, "lib/site-packages"))
    import pydevd
    pydevd.settrace('localhost', port=44444, stdoutToServer=True, stderrToServer=True)


A Remote Debugging configration needs to be setup to use the ``44444`` port (can be changed, but it needs to be reflected in the code), and running before the tests are run!

The ``pydevd`` package does not have to be installed on Python 2. As of 12/11/2018 installing ``pydevd`` on a separate installation with Python 3.7, and adding the code above successfully connects.


Setting up PyCharm on Linux
###########################

1. Use the native python interpreter (``/usr/bin/python2.7/python.exe``) rather than from ``<Mantid Source Directory>/external/src/ThirdParty/lib/python2.7/python.exe``
2. In the ``Project Structure`` sub menu you should see your root directory with the source/build directories both visible (if not, add them). The folder structure should be present in the centre of the window allowing you to mark folders orange (excluded) or blue (source). Source directories will be searched for python code.

   Within the source directory add the following to your sources:

   .. code-block:: sh

       <Mantid Source Directory>/Framework/PythonInterface
       <Mantid Source Directory>/scripts
       <Mantid Source Directory>/qt/applications/workbench
       <Mantid Source Directory>/qt/widgets
       <Mantid Source Directory>/qt/python


   The first folder can be replaced with the folder that contains your code, if you aren't writing code in ``scripts/``.

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
