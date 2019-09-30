.. _workbench:

=========
Workbench
=========

.. toctree::
   :hidden:
   :glob:

   *

.. figure:: ../images/Workbench/WorkbenchOverview.png
    :width: 500
    :alt: Workbench
    :align: right

    Overview of the workbench

Mantid Workbench is the newest user interface for Mantid. The Workbench will be
included alongside MantidPlot for several releases before replacing it completely. The Workbench has been built from the
ground up to be easier to use, more stable, support automatic testing and allow future development and changes to be
completed much faster than they were in MantidPlot.  In addition it is built on more up to date toolkits that will
allow us to keep developing the Workbench long after MantidPlot is no longer available.

**What is in Workbench?**
    * :ref:`WhatIsInWorkbench` This is a summary of what is in Workbench, including a diagram describing what is
      each aspect of the window.
    * :ref:`WorkbenchSmallerFeatures` A list of smaller features that does not warrant it's own pages including keyboard
      shortcuts.
    * :ref:`WorkbenchWhatIsNew` Lists the new features in Workbench as opposed to MantidPlot.

**Workbench Features**
    * :ref:`WorkbenchMainWindowMenu` Overview of the top-level menus.
    * :ref:`WorkbenchSettings` The control panel for settings in workbench.
    * :ref:`WorkbenchWorkspaceToolbox` The area in which all workspaces currently loaded into Workbench can be accessed
      and edited from.
    * :ref:`WorkbenchAlgorithmToolbox` shows a list is all of the algorithms available to users to run on workspaces loaded into
      the Workspace Toolbox.
    * :ref:`WorkbenchScriptWindow` provides the ability to edit and execute Python scripts.
    * :ref:`WorkbenchMessagesWindow` Display mantid log messages and Python output.
    * :ref:`WorkbenchPlotsToolbox` A way of controlling all available plots.
    * :ref:`WorkbenchIPythonConsole` provides an IPython console allowing the immeidate execution of Python commands
    * :ref:`WorkbenchPlotWindow` The new way that plots are created and manipulated in Workbench, includes plot options
      and fitting.
    * *Workspace data views*: Display data from ``MatrixWorkspace`` / ``TableWorkspace`` and edit ``TableWorkspace``
    * `Instrument view <https://www.mantidproject.org/MantidPlot:_Instrument_View>`_: Visualize an instrument attached to a workspace
    * *Sample log viewer*: Display information, plots and statistics about the sample logs in a workspace
    * :ref:`WorkbenchWorkspaceHistoryWindow` displays the algorithms that have been applied to a workspace
    * :ref:`Plotting <plotting>` describes how mantid workspaces have been integrated with the matplotlib framework and
      how to create these plots via scripts.
