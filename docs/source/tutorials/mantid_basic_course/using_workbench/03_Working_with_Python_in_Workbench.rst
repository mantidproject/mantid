.. _03_Working_with_Python_in_Workbench:

================================
Working with Python in Workbench
================================

Within the Mantid Workbench you can make use of the script window or IPython prompt to manipulate data and build plots exactly to your specifications in ways that you would not 
be able to with the interface alone.

.. figure:: /images/Workbench_script_new.png
   :width: 700px
   :alt: A view of the Workbench script window.
   
You can also use the IPython tab below the script editor to change to an inline Python interpreter. Both the python interpreter and scripts run thorugh Mantid are able to interact 
with Mantid workspaces, plot and algorithms.

1. Default imports
==================

By default any new script opened in Workbench will come with some default imports:

.. code-block:: python

   from __future__ import (absolute_import, division, print_function, unicode_literals)
   
This provideds compatability between scripts written in Python2 and Python3, notably 
all print statements must not use brackets.

.. code-block:: python

   from mantid.simpleapi import *
   
This imports all the algorithems from Mantid to be used within your Python script.

.. code-block:: python

   import matplotlib.pyplot as plt
   import numpy as np

This imports Matplotlib for use as well the the excellent NumPy package.

2. Tabs and multiple scripts
============================

To open a new script you can click on the **+** button, or to open an existing script in a new tab go to “File” -> “Open Script” in the Workbench taskbar or use the hotkey `Ctrl + o`.
From the script window you can also tab different scripts working on each independanty and multiple scripts can be run simultainiusly. Workbench will restore any tabs that were open
the last time Mantid was shut down.

3. Running, aborting, and options
===================================
From here you can run or abort your script, toggle options for accesability, and use the find and replace tool.

.. figure:: /images/Workbench_script_options.png
   :width: 700px
   :alt: A view of the Workbench script options.

* Run: This executes the currently selected text in your script, if nothing is selected it will run the entire script. The green play button serves as a shortcut for this option.
* Run All: This will run the entire script regardless of whether any text is selected.
* Abort: This imediatly aborts the currently running script. The red stop button serves as a shortcut for this option.
* Find/Replace: this opens the find and replace toolbar which can be used to make quick mass changes to your script or find sections of code.
* Comment/Uncomment: This tool comments out or removes commenting on highlighted lines by adding or removing ``#`` at the begining of the line.
* Toggle Whitespace visible: Turning this on will make all spaces apear as faint dots and all tabs appear as arrows.
* Tabs to spaces/Spaces to tabs: This will convert any tabs highlighted into groups of 4 white spaces or vice versa.

The toggle whitespace visible option is global and will effect the apperance of all your tabs.

4. Status
=========
The status bar tells you if the code in the currently open tab is running or not. If the code has been run previously the status bar will give deatils of the previous run including 
runtime, and whether the code ran without errors.