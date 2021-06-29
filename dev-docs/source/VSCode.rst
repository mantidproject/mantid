.. _VSCode:

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
- Navigate to your Mantid source directory and select it
- Install the required extensions (see below)
- For code editing you are good to go!

Extensions
==========
All Extensions have been tested working on Ubuntu 18.04 and Ubuntu 19.04,
however most, if not all, extensions should be cross-platform.

Install extensions either by running the commands given on the marketplace or by
clicking `the Extension Marketplace icon. <https://code.visualstudio.com/docs/editor/extension-gallery#_browse-for-extensions>`_

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
It is required in the mantid project to format code using clang-format, this can be done
in multiple ways but VSCode makes it easier. With a formatter installed as an extension
it is possible to format any open file in VSCode. It is recommended to use this `Clang-Format <https://marketplace.visualstudio.com/items?itemName=xaver.clang-format>`_ extension.
It is possible to format your code using on save, type, paste and save timeout. To set
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
This is where VSCode comes into its own as a versatile environment for programming
in any language, because of this it has a Tasks list. These tasks are saved in
``tasks.json`` inside the ``.vscode`` folder now present in any opened folder/workspace.

- To start creating a task open the command line (Ctrl+Shift+P or ⌘+Shift+P)
- Type "Task"
- Select "Task: Configure Task" by navigating using the arrow keys
- Select the first option that is in the menu and it should open tasks.json

The tasks.json allows the creation of Build Tasks default or otherwise. VSCode will
attempt to make a useful task for you. However it may be better to use this example:

**Linux/OSX:**
The commands can be switched out with the command and various args for the generator
used to generate your CMake with.

.. code-block:: javascript

    {
        "version": "2.0.0",
        "tasks": [
            {
                "label": "Build Mantid",
                "type": "shell",
                "command": "cmake",
                "args": [
                    "--build",
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
``launch.json`` alongside the ``tasks.json`` in the ``.vscode`` folder.

**If you want to debug/launch Mantid Workbench, please consider using PyCharm as that
is not covered here.**

To get to this file:
- Open commandline line (Ctrl+Shift+P or ⌘+Shift+P)
- Type "Debug: Open launch.json"
- Hit Enter.

If this fails
- Click on `the debug icon <https://code.visualstudio.com/docs/editor/debugging#_start-debugging>`_ on the left hand side of VSCode.
- Click `the cog icon at the top <https://code.visualstudio.com/docs/editor/debugging#_launch-configurations>`_ of this newly opened side window
- Select "(GDB) Launch" or "(msvc) Launch"

**Linux/OSX**

For this section the guide will show you how to use GDB debugging. Inside the launch.json
you will want to make your file look something a little like this:

*MantidPlot*

.. code-block:: javascript

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


*Workbench*

To debug C++ and start directly into the Workbench, add this to the configuration list in ``launch.json``.

.. code-block:: javascript

    {
      "name": "(gdb) Workbench C++ Only",
      "type": "cppdbg",
      "request": "launch",
      "program": "/usr/bin/python2.7", // Path to your used Python interpreter, here and below
      "args": ["Path/To/Build/Directory/bin/workbench", "&&","gdb","/usr/bin/python2.7","$!"], // $! gets the process ID
      "stopAtEntry": false,
      "cwd": "Path/To/Build/Directory/bin", // this should point to bin inside the build directory
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "preLaunchTask": "Build Mantid",
      "setupCommands": [
        {
          "description": "Enable pretty-printing for gdb",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    }



**Windows:**

For this section of the guide it will discuss use of the MSVC debugger. Please
follow on with the `guide <https://code.visualstudio.com/docs/cpp/config-msvc>`_.
The launch.json should end up looking a little like this:

.. code-block:: javascript

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

To actually start the debug session, switch to `the debug tab <https://code.visualstudio.com/docs/editor/debugging#_start-debugging>`_
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

.. code-block:: javascript

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

.. code-block:: javascript

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

.. code-block:: javascript

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

Debugging Python
-----------------
Visual Studio Code can be remotely attached to any running Python targets
using `debugpy`.
Whilst this "just works" for the majority of cases, it will not allow you to
debug both C++ and Python at the same time. It also will not work with
PyQt listeners, as the debugger must be attached to the main thread.

**Setting up debugpy**

*Linux/OSX*

Install `debugpy` using pip within the terminal

.. code-block:: bash

   python3 -m pip install --user debugpy

*Windows*

- Go to your source folder with Mantid (not the build folder)
- Go to external/src/ThirdParty/lib/python3.8
- Open a command prompt here (shift + right click in empty space)
- Run the following: `python -m pip install --user debugpy`

**Setting up VS Code**
- Ensure the Python extension is installed
- Open `launch.json` through either the debug tab or the file finder
- Add the following target

.. code-block:: javascript

    {
        "name": "Python Attach",
        "type": "python"
        "request": "attach"
        "port" : 5678,
        "host": "localhost"
    }

**Attaching the debugger**
- Go to the location where you would like Mantid to first trigger a breakpoint
- Insert the following code:

.. code-block:: python

    import debugpy
    debugpy.listen(('127.0.0.1', 5678))
    debugpy.wait_for_client()
    debugpy.breakpoint()

- When Mantid appears to freeze. Open the debug tab and start the "Python Attach" Target
- Any additional breakpoints using the IDE are added automatically
  (i.e. don't add `debugpy.breakpoint()`
- If you'd like the code to not break at that location, but would like the
  debugger to attach only remove `wait_for_client()`


Keybindings
-----------

To get a list of all of possible keybindings the open your command line
(Ctrl+Shift+P or ⌘+Shift+P) and search for "Help: Keyboard Shortcuts
Reference" and hit Enter.

**Very commonly used keybindings:**

+-------------------+---------------+---------------+---------------+
| Function          | Linux         | MacOS         | Windows       |
+===================+===============+===============+===============+
| Search in File    | Ctrl+F        | ⌘+F           | Ctrl+F        |
+-------------------+---------------+---------------+---------------+
| Command Line      | Ctrl+Shift+P  | ⌘+Shift+P     | Ctrl+Shift+P  |
+-------------------+---------------+---------------+---------------+
| Fuzzy File Search | Ctrl+P        | ⌘+P           | Ctrl+P        |
+-------------------+---------------+---------------+---------------+
| Build             | Ctrl+Shift+B  | Ctrl+Shift+B  | Ctrl+Shift+B  |
+-------------------+---------------+---------------+---------------+
| Launch            | F5            | F5            | F5            |
+-------------------+---------------+---------------+---------------+

Remote Development
------------------
VSCode supports the ability to open and work from directories on a remote machine using SSH.

Detailed instructions on how to set this up can be found `here <https://code.visualstudio.com/docs/remote/ssh>`_.

(Advanced) Getting Live Warnings and Errors with Clangd
=======================================================
(Linux only)

The C++ extension in VS Code provides limited inspection: it (currently) has
warnings disabled and will only emit errors.

Clang can be used to provide live warnings and will notify on common bugs, like
implicit casts, which are normally only detected whilst building.

Future versions of clangd (>=10) will also emit clang-tidy warnings as you
work.

**Setup**

- Remove the C++ Intellisense extension
- Remove the C++ extension and install Native Debug to keep C++ debugging OR
-  Go to the C++ extension settings and disable the following:

  Autocomplete, Enhanced Colorization, Error Squiggles,
  Experimental Features, IntelliSense Engine, IntelliSense Engine Fallback

- Install the official clangd extension: `vscode-clangd`
- Install clangd >= 8 which is part of `clang-tools-n`
  (where n is the latest version)
- Create a folder for a clang build separate to your main Mantid build.
  One recommended location is to create it in a folder called **build**
  within the source folder since this will also be rebuilt by the
  *CMakeTools* extension, if you have it.
- Configure this separate folder to use the clang compiler:

.. code-block:: sh

    cd *path/to/clang_build*
    CXX=clang++ CC=clang cmake *path/to/src* -DPYTHON_EXECTUABLE=/usr/bin/python2  # (or 3)
    # Note this does not have to build unless you want to!

- Go to the clangd setting in VS Code and add the following argument:
  `--compile-commands-dir=/path/to/your/clang-build` ensuring that build
  folder is related to the source folder. This allows clangd to understand
  the structure of Mantid.
- Restart VS Code - attempt to write: `int i = (size_t) 1;` and check a warning
  appears.
- Any errors about unknown types can usually be resolved by briefly opening
  that header to force clangd to parse the type.
