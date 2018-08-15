.. _GettingStartedWithPyCharm:

Getting Started with PyCharm
============================

PyCharm can be installed from `here <https://jetbrains.com/pycharm/download/>`_.

This tutorial assumes you are familiar with the process of building Mantid (with separate source and build directories inside a root directory), and that you have built a working version. If you are unclear about this see `here <GettingStarted.html>`__

.. contents::
  :local:

Setting up PyCharm on Windows
#############################

PyCharm should be opened using ``pycharm.bat`` which can be found in the build directory (this sets some additional environment variables compared with simply opening PyCharm directly).

1. Once PyCharm is open, set up the project. Go to ``File->Open`` and select the root directory in which both your source and build directories reside.

   Go to ``File->Settings``, then under ``Project`` you will set two sub-menus ``Project Interpreter`` and ``Project Structure``. The interpreter defines the python executable that will be used to run your code, and the structure menu allows you to decide which folders within the project to include and index.

2. In the ``Project Interpreter`` sub menu, at the top select the options button and click ``Add...``, a new window should appear titled "Add Python Interpreter". In the menu on the left, select "System Interpreter" (a version of Python with all the correct variables set already exists within Mantid). Click on the ``...`` to open a file browser, and navigate to;

    .. code-block:: sh

        <Mantid Source Directory>/external/src/ThirdParty/lib/python2.7/python.exe

   This is the interpreter, so select "Ok" and apply the changes. This should bring up a list of all the packages associated to the interpreter. There should be many packages, however you should not see PyQt (but instead QtPy).

3. In the ``Project Structure`` sub menu you should see your root directory with the source/build directories both visible (if not, add them). The folder structure should be present in the centre of the window allowing you to mark folders orange (excluded) or blue (source). Source directories will be searched for python code.

   Within the source directory add the following to your sources;

    .. code-block:: sh

        <Mantid Source Directory>/scripts
        <Mantid Source Directory>/external/src/ThirdParty/lib

   The first folder can be replaced with the folder that contains your code, if you aren't writing code in ``scripts/``. In the Mantid build directory add the following as source folders;

    .. code-block:: sh

        <Mantid Build Directory>/bin/Debug

   here we are setting up PyCharm for the Debug build, you would use ``/bin/Release`` instead if you are building mantid in release mode.

NOTE : In some cases, imports in the code will still be highlighted red when they come from folders within the ``script/`` folder, or from other folders entirely. To fix this simply add the relevant folder that contains the module you are importing in the same fashion as step 3 above. 

Running Files in the Debugger
-----------------------------

Running python code from within PyCharm which depends on the python API, or PyQt for example requires one extra step. Because the source root labelling from the previous section only affects PyCharm searching and not the run configuration, before running the file we must set up the run configuration correctly.

As an example, create a new file in ``<Mantid Source Directory>/scripts/`` called ``test.py``. Copy into it the Python code below.

4. To edit the configurations go to ``Run->Run...`` and select ``Edit Configurations``. This should open up a sub window. Hit the green ``+`` in the top left to create a new configuration and name it. In order to tell PyCharm where to look for python modules and libraries we need to add some folders to the ``PATH`` environment variable. Click on the ``...`` next to the *Environment Variables* box, and hit the ``+`` icon. In the Name column enter "PATH", in the value column enter the following;

    .. code-block:: sh

        <Mantid Build Directory>\bin\Debug;
        <Mantid Source Directory>\external\src\ThirdParty\bin;
        <Mantid Source Directory>\external\src\ThirdParty\lib\qt4\bin;
        <Mantid Source Directory>\external\src\ThirdParty\lib\qt5\bin;
        %PATH%

   The semi-colon delimited list of paths should end in ``;%PATH%`` so that we prepend to the existing list of paths rather than overwriting them. The last two lines will allow imports of PyQt4 and PyQt5 modules.

You should now be able to run and debug the scripts using the newly created configuration, by adding the full path of the file in the ``Script path`` box at the top of the configuration window.


Testing using PyQt
------------------

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

Setting up PyCharm on Linux
###########################

On Linux the instructions are identical to Windows except that :

- In step 1, the file is ``pycharm.sh`` rather than ``pycharm.bat``
- In step 2, use the native python interpreter (``/usr/bin/python2.7/python.exe``) rather than from ``<Mantid Source Directory>/external/src/ThirdParty/lib/python2.7/python.exe``
- In step 4, add ``<Mantid Build Directory>/bin;`` to the ``PATH`` environment variable in the new configuration (rather than ``<Mantid Build Directory>/bin/Debug;``), and remove the other three file paths.

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
