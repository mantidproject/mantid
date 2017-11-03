Scripting Window
================

.. contents:: Table of Contents
  :local:

Overview
--------

.. image:: /images/ScriptingWindow.png

The scripting window allows you to write and execute Python scripts that interact
with the Mantid :ref:`Python API <api>`. It can be accessed either by selecting the
*Script Window* option from the *View* menu in MantidPlot or by pressing F3.

Editor Options
--------------

The general options for the script editor are controlled using the *Edit* and
*Window* menus.

Alongside the standard text copy and paste tools the *Edit* menu has tools that
help format Python code, all of these tools can operate over a subsection of the
code which is selected by highlighting it in the editor.

Comment
  Comments out the selected lines or the current line if no code is highlighted.

Uncomment
  Uncomments the selected lines or or the current line if no code is
  highlighted.

Tabs to Spaces
  Converts any tabs in the current selection (or the entire script if no code is
  highlighted) to spaces.

Spaces to Tabs
  Converts any spaces in the current selection (or the entire script if no code
  is highlighted) to tabs.

The *Window* menu contains options relating to the way in which code is
displayed in the editor:

Always On Top
  Enabling this option ensures that the scripting windows will stay above all
  other windows.

Progress Reporting
  This option enables the green arrow which points to the current line being
  executed on the left hand side of the editor.

.. image:: /images/ScriptingWindow_FoldingOptionEnabled.png
   :align: right
   :scale: 50%

Code Folding
  This option adds fold markers next to the line numbers on the right hand side
  of the editor which allow blocks of code to be collapsed based on their
  indentation levels.

.. image:: /images/ScriptingWindow_WhitespaceOptionEnabled.png
   :align: right
   :scale: 50%

Show Whitespace
  This option is used to show the whitespace in the editor window, this can be
  very useful for finding indentation issues.

.. image:: /images/ScriptingWindow_ConfigureTabs.png
   :align: right

Configure Tabs
  This dialog is used to edit the way in which tabs are converted to and from
  spaces by the *Tabs to Spaces* and *Spaces to Tabs* tools as well as how stabs
  are displayed in the editor window.
  It is recommended to leave these settings set to enable *Replace tabs with
  spaces* and *Number of spaces per tab* set to 4.

Execution Options
-----------------

The *Execution* menu contains options regarding the execution of the script, the
two main options here are *Execute All* and *Execute Selection*.

Execute All
  This option will execute the entire script from the top down in the standard
  way a Python script would be executed.

Execute Selection
  This option allows selection of a subset of the code to be executed, this is
  selected by highlighting the lines you wish to execute in the editor window
  and selecting the option.

Note that when using the *Execute Selection* option you must have the entire line
selected as the code is taken from the first highlighted character to the last.
For this reason there is also a limitation that prevents you from executing a
selection where the first line is indented at any level as this will generate an
indentation warning from the Python interpreter since it is effectively seeing
an indented block where it would not expect to.

The *Clear Variables* option is used to clear the local variables in the current
script, this is usually not something that would need to be done as it is done
automatically before each execution of a script.

The *Mode* option allows you to switch between running the script asynchronously
or serialised with the UI, typically running scripts serialised is not
recommended as you will lose control of the MantidPlot UI whilst the script is
being executed.

Script Output
-------------

The output pane at the bottom of the window displays the output from the script
in the current tab, this shows the times execution of the script has started
and finished as well as anything output from the script using :code:`print`
statements.

.. image:: /images/ScriptingWindow_OutputOptions.png

Right clicking on the output pane opens the context menu which allows you to
export the output in multiple ways as well as clearing the window.

.. categories:: Interfaces
