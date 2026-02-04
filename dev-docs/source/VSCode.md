# VSCode

```{contents}
---
local:
---
```

## Introduction

This is a guide aimed at the developer who does not want a heavy complicated IDE
for C++ development on any platform that VSCode supports

## Prerequisites

- An installed copy of VSCode - [download](https://code.visualstudio.com/).
- A clone of the main Mantid repo
- Follow the [Getting Started](GettingStarted/GettingStarted) guide to get a:
- Built copy of Mantid (In "Debug" mode for any debugging option not "Release")
- GDB for Linux/OSX debugging (GDB can be switched out for LLDB at any stage on OSX) or MSVC (Visual Studio) for Windows

## How to get Started

- Run VSCode
- Click File->Open Folder
- Navigate to your Mantid source directory and select it
- Install the required extensions (see below)
- For code editing you are good to go!

## Extensions

All Extensions have been tested working on Ubuntu 18.04 and Ubuntu 19.04,
however most, if not all, extensions should be cross-platform.

Install extensions either by running the commands given on the marketplace or by
clicking [the Extension Marketplace icon.](https://code.visualstudio.com/docs/editor/extension-gallery#_browse-for-extensions)

### Required

- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  - Required for C++ linting, debugging etc
- [Python](https://marketplace.visualstudio.com/items?itemName=ms-python.python)
  - Required to run any python code, debug etc

### Recommended

- [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)

  - Allows auto formatting using the clang format version installed on your machine

- [CMake](https://marketplace.visualstudio.com/items?itemName=twxs.cmake)

  - Allows language highlighting and auto fill for CMake

- [Doxygen Document Generator](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen)

  - Once a function is written with this extension it can generate parts of the doxygen comments itself

- [GitLens](https://marketplace.visualstudio.com/items?itemName=eamodio.gitlens)

  - Very powerful source control assistant

- [Visual Studio Intellicode](https://marketplace.visualstudio.com/items?itemName=VisualStudioExptTeam.vscodeintellicode)

  - This improves your code suggestions in certain languages

## Features

### Auto Formatting

It is required in the mantid project to format code using clang-format, this can be done
in multiple ways but VSCode makes it easier. With a formatter installed as an extension
it is possible to format any open file in VSCode. It is recommended to use this [Clang-Format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format) extension.
It is possible to format your code using on save, type, paste and save timeout. To set
when you want to format:

- Open the File->Preferences->Settings menu
- Search for "Format On"
- Then tick the times at which formatting should occur. (recommend save only)

### Auto Saving

This is a built in feature for VSCode much like with formatting however no extension
is required. You can save on window change, on focus change, and on delay.

- Open the File->Preferences->Settings menu
- Search for "Auto Save"
- Select the auto save type that is wanted (recommend `onFocusChange`)

### Building

This is where VSCode comes into its own as a versatile environment for programming
in any language, because of this it has a Tasks list. These tasks are saved in
`tasks.json` inside the `.vscode` folder now present in any opened folder/workspace.

- To start creating a task open the command line (Ctrl+Shift+P or ⌘+Shift+P)
- Type "Task"
- Select "Task: Configure Task" by navigating using the arrow keys
- Select the first option that is in the menu and it should open tasks.json

The tasks.json allows the creation of Build Tasks default or otherwise. VSCode will
attempt to make a useful task for you. However it may be better to use this example:

**Linux/OSX:**
The commands can be switched out with the command and various args for the generator
used to generate your CMake with.

```javascript
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
```

**Windows:**

For Windows you should seriously consider using the IDE Visual Studio. However if
you are sure that you want VSCode it makes most sense to checkout this
[guide](https://code.visualstudio.com/docs/cpp/config-msvc).

**Actually Building:**

- Now to build with this task open the command line again
- Type "Task"
- Select "Tasks: Run Build Task"

### Debugging and Launching

Debugging is similar to Building in the sense that you complete a task that has been
defined. For Debugging and Launching all of these 'Tasks' are stored in the
`launch.json` alongside the `tasks.json` in the `.vscode` folder.

**If you want to debug/launch Mantid Workbench, please consider using PyCharm as that
is not covered here.**

To get to this file:

- Open commandline line (Ctrl+Shift+P or ⌘+Shift+P)
- Type "Debug: Open launch.json"
- Hit Enter.

If this fails

- Click on [the debug icon](https://code.visualstudio.com/docs/editor/debugging#_start-debugging) on the left hand side of VSCode.
- Click [the cog icon at the top](https://code.visualstudio.com/docs/editor/debugging#_launch-configurations) of this newly opened side window
- Select "(GDB) Launch" or "(msvc) Launch"

**Linux/OSX**

For this section the guide will show you how to use GDB/LLDB debugging.
Inside the launch.json you will want to make your file look something a little like this:

*Workbench*

To debug C++ and start directly into the Workbench, add this to the configuration list in `launch.json`.

```javascript
{
    "name": "(gdb) Workbench C++ Only",
    "type": "cppdbg",
    "request": "launch",
    "program": "/Path/To/Mamba/Install/envs/mantid-developer/bin/python",   // Full path (do not use '~') to the python executable inside your build directory
    "preLaunchTask": "Build Mantid",
    "args": ["-m", "workbench", "--single-process"],
    "MIMode": "gdb",
    "cwd": "Path/To/Build/Directory/bin", // this should point to bin inside the build directory
    "stopAtEntry": false,
    "setupCommands": [
        {
        "description": "Enable pretty-printing for gdb",
        "text": "-enable-pretty-printing",
        "ignoreFailures": true
        }
    ]
}
```

The `--single-process` flag is necessary for debugging. See the `RunningWorkbench` documentation for more information.

If this fails, try adding the following environment variables:

```javascript
"environment": [
  {"name":"LD_PRELOAD", "value": "/usr/lib/x86_64-linux-gnu/libjemalloc.so.1"},
  {"name":"PYTHONPATH", "value": "Path/To/Build/Directory/bin:${env:PYTHONPATH}"}
],
```

where the correct value for the `LD_PRELOAD` environment variable can be found in Path/To/Build/Directory/bin/launch_mantidworkbench.sh.

```{include} macos-opengl-version-warning.md
```

**Windows:**

For this section of the guide it will discuss use of the MSVC debugger. Please
follow on with the [guide](https://code.visualstudio.com/docs/cpp/config-msvc).
The launch.json should end up looking a little like this:

```javascript
{
   "version": "0.2.0",
    "configurations": [
        {
            "name": "(msvc) Launch",
            "type": "cppvsdbg",
            "request": "launch",
            "program": "Path/To/Build/Directory/bin/Debug/MantidWorkbench.exe",
            "args": [],
            "stopAtEntry": true,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "preLaunchTask": "Build Mantid" // This causes the task labelled to be called before
        }
    ]
}
```

To actually start the debug session, switch to [the debug
tab](https://code.visualstudio.com/docs/editor/debugging#_start-debugging)
and select "(GDB) Launch" from the drop down and click the play button.

### Debugging C++ called from Workbench

**Linux/OSX:**

To achieve this we will use the GDB debugger's ability to attach itself to a
process. To do this we will need it's ProcessID. There are various ways to get this
its recommended to launch workbench from PyCharm in Debug mode and grabbing the ID
from the Debug terminal window.

In your launch.json we will need a new launch task for this, this new task should look
like this:

```javascript
{
    "name": "(gdb) Launch Workbench",
    "type": "cppdbg",
    "request": "launch",
    "program": "/usr/bin/python3",
    "args": [
        "/path/to/build/dir/bin/MantidWorkbench"
    ],
    "MIMode": "gdb",
    "cwd": "${fileDirname}",
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        }
    ]
}
```

- Place this json in the "configurations" list in launch.json
- Then launch the debug session like any other, note it may be slow to get started.

### Debugging C++ Tests

**Linux/OSX**

First thing to do is make sure that the test you are testing is built. You can do this
by building via one of the test targets. An example Task for AlgorithmsTest:

```javascript
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
```

To debug the individual tests you won't want to be running all tests, so you will need to
select the executable for your tests i.e. "bin/AlgorithmsTest" in your build directory.
Then pass as an argument the specific test you want to be debugging. As an example:

```javascript
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
```

### Debugging Python

Visual Studio Code can be remotely attached to any running Python targets
using `debugpy`.
Whilst this "just works" for the majority of cases, it will not allow you to
debug both C++ and Python at the same time. It also will not work with
PyQt listeners, as the debugger must be attached to the main thread.

**Setting up debugpy**

*Linux/OSX*

Install `debugpy` using pip within the terminal

```bash
python3 -m pip install --user debugpy
```

*Windows*

- Go to your source folder with Mantid (not the build folder)
- Go to external/src/ThirdParty/lib/python3.8
- Open a command prompt here (shift + right click in empty space)
- Run the following: `python -m pip install --user debugpy`

**Setting up VS Code**

- Ensure the Python extension is installed
- Open `launch.json` through either the debug tab or the file finder
- Add the following target

```javascript
{
    "name": "Python Attach",
    "type": "python"
    "request": "attach"
    "port" : 5678,
    "host": "localhost"
}
```

**Attaching the debugger**

- Go to the location where you would like Mantid to first trigger a breakpoint
- Insert the following code:

```python
import debugpy
debugpy.listen(('127.0.0.1', 5678))
debugpy.wait_for_client()
debugpy.breakpoint()
```

- When Mantid appears to freeze. Open the debug tab and start the "Python Attach" Target
- Any additional breakpoints using the IDE are added automatically
  (i.e. don't add `debugpy.breakpoint()`)
- If you'd like the code to not break at that location, but would like the
  debugger to attach only remove `wait_for_client()`

### Keybindings

To get a list of all of possible keybindings the open your command line
(Ctrl+Shift+P or ⌘+Shift+P) and search for "Help: Keyboard Shortcuts
Reference" and hit Enter.

**Very commonly used keybindings:**

| Function          | Linux        | MacOS        | Windows      |
| ----------------- | ------------ | ------------ | ------------ |
| Search in File    | Ctrl+F       | ⌘+F          | Ctrl+F       |
| Command Line      | Ctrl+Shift+P | ⌘+Shift+P    | Ctrl+Shift+P |
| Fuzzy File Search | Ctrl+P       | ⌘+P          | Ctrl+P       |
| Build             | Ctrl+Shift+B | Ctrl+Shift+B | Ctrl+Shift+B |
| Launch            | F5           | F5           | F5           |

### Remote Development

VSCode supports the ability to open and work from directories on a remote machine using SSH.

Detailed instructions on how to set this up can be found [here](https://code.visualstudio.com/docs/remote/ssh).

## (Advanced) Getting Live Warnings and Errors with Clangd

(Linux only)

The default C++ extension in VS Code provides limited inspection: it has
warnings disabled and will only emit errors.

Clang can be used to provide live warnings and runs clang-tidy continuously. This helps detect warnings and errors live which are normally only detected whilst building.

**Setup**

- Install the latest stable [clang ppa](https://apt.llvm.org/)
- Install the clangd extension
- Install the latest stable clangd, e.g. `clang-tools-n` (where n is the latest version)
- Go to the clangd settings in VS Code and ensure the correct binary is manually specified
- Restart VS Code - attempt to write: `int i = (size_t) 1;` and check a warning appears.
