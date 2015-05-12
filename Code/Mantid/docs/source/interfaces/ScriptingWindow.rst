Scripting Window
================

.. contents:: Table of Contents
  :local:

Overview
--------

.. image:: /images/ScriptingWindow.png

TODO

Editor Options
--------------

TODO

.. image:: /images/ScriptingWindow_FoldingOptionEnabled.png

.. image:: /images/ScriptingWindow_WhitespaceOptionEnabled.png

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

Note that when using th *Execute Selection* option you must have the entire line
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

.. image:: /images/ScriptingWindow_OutputOptions.png

TODO

.. categories:: Interfaces
