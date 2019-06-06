.. _VSCode:

.. |extensions| image:: ../images/VSCode/extension-button.png
.. |debug| image:: ../images/VSCode/debug-button.png
.. |debug-cog| image:: ../images/VSCode/debug-cog-button.png

======
VSCode
======

.. contents::
    :local:

Introduction
============
This is a guide aimed at the developer who does not want a heavy complicated IDE
for C++ development on any platform that VSCode supports

Prerequisites
=============
- An installed copy of VSCode - `download <https://code.visualstudio.com/>`_.
- A clone of the main Mantid repo
- Follow the :ref:`GettingStarted` guide to get a:
- Built copy of Mantid (In "Debug" mode for any debugging option not "Release")
- GDB for Linux/OSX debugging (GDB can be switched out for LLDB at any stage on OSX) or MSVC (Visual Studio) for Windows

How to get Started
==================
- Run VSCode
- Click File->Open Folder
- Navigate to your clone of the Mantid Repo and select it
- Install the required extensions
- For code editing you are good to go!

Extensions
==========
All Extensions have been tested working on Ubuntu 18.04 LTS and Ubuntu 19.04,
however most, if not all, extensions should be cross-platform.

Install extensions either by running the commands given on the marketplace or by
clicking on this Icon |extensions|.

Required
--------
- `C/C++ <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>`_
    - Required for C++ linting, debugging etc
- `Python <https://marketplace.visualstudio.com/items?itemName=ms-python.python>`_
    - Required to run any python code, debug etc

Recommended
-----------
- `Clang-Format <https://marketplace.visualstudio.com/items?itemName=xaver.clang-format>`_
    - Allows auto formatting using the clang format version installed on your machine
- `CMake <https://marketplace.visualstudio.com/items?itemName=twxs.cmake>`_
    - Allows language highlighting and auto fill for CMake
- `Doxygen Document Generator <https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen>`_
    - Once a function is written with this extension it can generate parts of the doxygen comments itself
- `GitLens <https://marketplace.visualstudio.com/items?itemName=eamodio.gitlens>`_
    - Very powerful source control assistant
- `Visual Studio Intellicode <https://marketplace.visualstudio.com/items?itemName=VisualStudioExptTeam.vscodeintellicode>`_
    - This improves your code suggestions in certain languages

Features
========

Auto Formatting
---------------
With a formatter installed as an extension it is recommended to use Clang-Format. It
is possible to format your code using on save, type, paste and save timeout. To set
when you want to format:
- Open the File->Preferences->Settings menu
- Search for "Format On"
- Then tick the times at which formatting should occur. (recommend save only)

Auto Saving
-----------
This is a built in feature for VSCode much like with formatting however no extension
is required. You can save on window change, on focus change, and on delay.
- Open the File->Preferences->Settings menu
- Search for "Auto Save"
- Select the auto save type that is wanted (recommend `onFocusChange`)

Building
--------
This is where VSCode comes into it's own as a versatile environment for programming
in any language, because of this it has a Tasks list. These tasks are saved in
tasks.json inside the .vscode folder now present in any opened folder/workspace.

- To start creating a task open the command line = Ctrl+Shift+P
- Type "Task"
- Select "Task: Configure Task" by navigating using the arrow keys
- Select the first option that is in the menu and it should open tasks.json

The tasks.json allows the creation of Build Tasks default or otherwise. VSCode will
attempt to make a useful task for you. However it may be better to use this example:

**Linux/OSX:**

This assumes that you have generated in CMake using the ninja the commands can be
switched out with the command and various args for whatever you have generated with.

.. code-block:: json

    {
        "version": "2.0.0",
        "tasks": [
            {
                "label": "Build Mantid",
                "type": "shell",
                "command": "ninja",
                "args": [
                    "-C",
                    "Build/Directory/Here"
                ],
                "group": {
                    "kind": "build",
                    "isDefault": true
                }
            }
        ]
    }

**Windows:**

For Windows you should seriously consider using the IDE Visual Studio. However if
you are sure that you want VSCode it makes most sense to checkout this
`guide <https://code.visualstudio.com/docs/cpp/config-msvc>`_.

**Actually Building:**

- Now to build with this task open the command line again
- Type "Task"
- Select "Tasks: Run Build Task"


Debugging and Launching
-----------------------
Debugging is similar to Building in the sense that you complete a task that has been
defined. For Debugging and Launching all of these 'Tasks' are stored in the
launch.json alongside the tasks.json in the .vscode folder.

**If you want to debug/launch Mantid Workbench, please consider using PyCharm as that
is not covered here.**

To get to this file:
- Open commandline line Ctrl+Shift+P
- Type "Debug: Open launch.json"
- Hit Enter.

If this fails
- Click on the debug icon on the left hand side of VSCode |debug|
- Click on the cod icon at the top of this newly opened side window |debug-cog|
- Select "(GDB) Launch" or "(msvc) Launch"

**Linux/OSX**

For this section the guide will show you how to use GDB debugging. Inside the launch.json
you will want to make your file look something a little like this:

.. code-block:: json

    {
        "version": "0.2.0",
        "configurations": [
            {
                "name": "(gdb) Launch",
                "type": "cppdbg",
                "request": "launch",
                "program": "Path/To/Build/Directory/bin/MantidPlot",
                "args": [],
                "stopAtEntry": false,
                "cwd": "${workspaceFolder}",
                "environment": [],
                "externalConsole": false,
                "MIMode": "gdb",
                "preLaunchTask": "Build Mantid", // This causes the task labelled to be called before
                "setupCommands": [
                    {
                        "description": "Enable pretty-printing for gdb",
                        "text": "-enable-pretty-printing",
                        "ignoreFailures": true
                    }
                ]
            }
        ]
    }

**Window:**

For this section of the guide it will discuss use of the MSVC debugger. Please
follow on with the `guide <https://code.visualstudio.com/docs/cpp/config-msvc>`_.
The launch.json should end up looking a little like this:

.. code-block:: json

    {
       "version": "0.2.0",
        "configurations": [
            {
                "name": "(msvc) Launch",
                "type": "cppvsdbg",
                "request": "launch",
                "program": "Path/To/Build/Directory/bin/Debug/MantidPlot.exe",
                "args": [],
                "stopAtEntry": true,
                "cwd": "${workspaceFolder}",
                "environment": [],
                "externalConsole": false,
                "preLaunchTask": "Build Mantid" // This causes the task labelled to be called before
            }
        ]
    }

To actually start the debug session, switch to the debug tab (clicking |debug|)
and select "(GDB) Launch" from the drop down and click the play button.

Debugging C++ called from Workbench
-----------------------------------
**Linux/OSX:**

To achieve this we will use the GDB debugger's ability to attach itself to a
process. To do this we will need it's ProcessID. There are various ways to get this
its recommended to launch workbench from PyCharm in Debug mode and grabbing the ID
from the Debug terminal window.

In your launch.json we will need a new launch task for this, this new task should look
like this:

.. code-block:: json

        {
            "name": "(gdb) Attach Workbench Python 2.7",
            "type": "cppdbg",
            "request": "attach",
            "program": "/usr/bin/python2.7", // Path to your used Python interpreter
            "processId": "1234", // Replace this with the process ID of workbench
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }

- Place this json in the "configurations" list in launch.json
- Then launch the debug session like any other

Debugging C++ Tests
-------------------

**Linux/OSX**

First thing to do is make sure that the test you are testing is built. You can do this
by building via one of the test targets. An example Task for AlgorithmsTest:

.. code-block:: json

    {
        "label": "Build Mantid AlgorithmsTest",
        "type": "shell",
        "command": "ninja",
        "args": [
            "-C",
            "Build/Directory",
            "AlgorithmsTest"
        ],
        "group": {
            "kind": "build",
            "isDefault": true
        }
    }

To debug the individual tests you won't want to be running all tests, so you will need to
select the executable for your tests i.e. "bin/AlgorithmsTest" in your build directory.
Then pass as an argument the specific test you want to be debugging. As an example:

.. code-block:: json

    {
        "name": "(gdb) Launch Ctest",
        "type": "cppdbg",
        "request": "launch",
        "program": "Build/Directory/bin/AlgorithmsTest",
        "args": [
            "RemoveSpectraTest" // This is the name of the test you want to Debug
        ],
        "stopAtEntry": false,
        "cwd": "Build/Directory",
        "environment": [],
        "externalConsole": false,
        "MIMode": "gdb",
        "preLaunchTask": "Build Mantid AlgorithmTests", // Once again this builds the task before doing debugging
        "setupCommands": [
            {
                "description": "Enable pretty-printing for gdb",
                "text": "-enable-pretty-printing",
                "ignoreFailures": true
            }
        ]
    }

Keybindings
-----------

To get a list of all of possible keybindings the open your command line
Ctrl+Shift+P and search for "Help: Keyboard Shortcuts Reference" and hit Enter.

**Very commonly used keybindings:**

+-------------------+---------------+
| Search in File    | Ctrl+F        |
+-------------------+---------------+
| Command Line      | Ctrl+Shift+P  |
+-------------------+---------------+
| Fuzzy File Search | Ctrl+P        |
+-------------------+---------------+
| Build             | Ctrl+Shift+B  |
+-------------------+---------------+
| Launch            | F5            |
+-------------------+---------------+
