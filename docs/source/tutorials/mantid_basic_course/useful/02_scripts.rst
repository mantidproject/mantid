.. _02_scripts:

===================
Working with Python
===================

This is a quick summary of how to use python scripts in MantidWorkbench. To learn more about this, see the further tutorials
on the `home page <https://www.mantidproject.org>`__.
Even if you are a novice at Python, it is useful to know how to run a script as you can produce one within Mantid:
from the "Generate a script" button on a plot toolbar, or from project recovery when you reopen MantidWorkbench after a crash.

Within MantidWorkbench you can make use of the script window or IPython prompt to manipulate data and build plots exactly to your specifications in ways that you would not
be able to with the interface alone.

.. figure:: /images/Workbench_script_new.png
   :width: 700px
   :alt: A view of the MantidWorkbench script window
   :align: center

You can also use the IPython tab below the script editor to change to an inline Python interpreter.
Both the python interpreter and scripts run through MantidWorkbench are able to interact
with Mantid workspaces, plot and algorithms.

1. Default imports
==================

By default any new script opened in MantidWorkbench will come with some default imports:

.. code-block:: python

   from mantid.simpleapi import *

This imports all the algorithms from Mantid to be used within your Python script.

.. code-block:: python

   import matplotlib.pyplot as plt
   import numpy as np

This imports Matplotlib for use as well as the excellent NumPy package.

In MantidWorkbench these imports are visibly added at the top of the script, so that script will also work outside of MantidWorkbench.

2. Tabs and multiple scripts
============================

To open a new script you can click on the **+** button, or to open an existing script in a new tab go to “File” -> “Open Script” in the MantidWorkbench taskbar or use the hotkey `Ctrl + O`.
From the script window you can also tab different scripts working on each independently and multiple scripts can be run simultaneously. MantidWorkbench will restore any tabs that were open
the last time MantidWorkbench was shut down.

3. Running, aborting, and options
===================================
From here you can run or abort your script, toggle options for accessibility, and use the find and replace tool.

.. figure:: /images/Workbench_script_options.png
   :width: 400px
   :alt: A view of the MantidWorkbench script options.
   :align: center

* Run: This executes the currently selected text in your script, if nothing is selected it will run the entire script. The green play button serves as a shortcut for this option.
* Run All: This will run the entire script regardless of whether any text is selected.
* Abort: This immediately aborts the currently running script. The red stop button serves as a shortcut for this option.
* Find/Replace: this opens the find and replace toolbar which can be used to make quick mass changes to your script or find sections of code.
* Comment/Uncomment: This tool comments out or removes commenting on highlighted lines by adding or removing ``#`` at the beginning of the line.
* Toggle Whitespace visible: Turning this on will make all spaces appear as faint dots and all tabs appear as arrows.
* Tabs to spaces/Spaces to tabs: This will convert any tabs highlighted into groups of 4 white spaces or vice versa.

The toggle whitespace visible option is global and will affect the appearance of all your tabs.

4. Status
=========
The status bar tells you if the code in the currently open tab is running or not. If the code has been run previously the status bar will give details of the previous run including
runtime, and whether the code ran without errors.

Useful Links
============

How to use a script to:

* :ref:`Control Workspaces <02_scripting_workspaces>`
* :ref:`Customise Plots <02_scripting_plots>`
