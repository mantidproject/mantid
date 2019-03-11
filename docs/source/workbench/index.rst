.. _workbench:

=========
Workbench
=========

.. toctree::
   :hidden:
   :glob:

   algorithmlistwidget.rst
   mainwindowmenu.rst
   messageswindow.rst
   plotstoolbox.rst
   plotwindow.rst
   scriptwindow.rst
   workspacetreewidget.rst
   whatisinworkbench.rst
   ipythonconsole.rst
   smallerfeatures.rst

.. image:: ../images/Workbench/Workbench.png
    :height: 225
    :width: 400
    :alt: Workbench
    :align: right

Mantid Workbench is the newest user interface for Mantid. The Workbench will be
included alongside MantidPlot for several releases before replacing it completely. The Workbench has been built from the
ground up to be easier to use, more stable, support automatic testing and allow future development and changes to be
completed much faster than they were in MantidPlot.  In addition it is built on more up to date toolkits that will
allow us to keep developing the workbench long after MantidPlot is no longer available.

**What is in Workbench?**
    * :ref:`WhatIsInWorkbench`
    * :ref:`WorkbenchSmallerFeatures`

**Workbench Features**
    * :ref:`WorkbenchWorkspaceToolbox`
    * :ref:`WorkbenchAlgorithmToolbox`
    * :ref:`WorkbenchScriptWindow`
    * :ref:`WorkbenchMessagesWindow`
    * :ref:`WorkbenchMainWindowMenu`
    * :ref:`WorkbenchPlotsToolbox`
    * :ref:`WorkbenchIPythonConsole`
    * :ref:`WorkbenchPlotWindow`

Script Window
-------------
.. image:: ../images/Workbench/Editor/EditorWidget.png

The :ref:`WorkbenchScriptWindow` is another key part of the workbench. The script window allows you to write a python
script which integrates with the Mantid framework and operate on the data loaded in via the workspaces. The script
window contains a Status bar, multiple tabs for different scripts, the ability to play, and the ability to stop
alongside some further options explored more here: :ref:`WorkbenchScriptWindow`

Message Window
--------------
.. image:: ../images/Workbench/MessageWindow/MessagesWidgetSmall.png

All of the output from your scripts and algorithm's ran from interfaces or other buttons will be output here. This is
alongside the ability change the logging level. The logging level has 5 different options to display the right level of
detail for you, all this and more is discussed further here: :ref:`WorkbenchMessagesWindow`

Main Window Menu
----------------

.. image:: ../images/Workbench/MainWindowMenu/MainWindowMenu.png

In your main window for workbench, you may notice a few options in the upper left hand corner. From here you can access
drop down menus which will facilitate options, to do things such as change the settings, save scripts, save projects,
restore defaults for the window and more. Further explanation can be found here: :ref:`WorkbenchMainWindowMenu`

Plot Toolbox
------------

.. image:: ../images/Workbench/PlotToolbox/PlotsToolbox.png
    :height: 200
    :width: 150
    :alt: Plots Toolbox
    :align: center

This is a new concept in Mantid applications, the plots toolbox will encompass and display all currently shown plots
much like the workspace toolbox. It has the option to hide, remove, and edit the name of each plot individually. All
this can be done from the buttons on top of the toolbox which will allow you to perform operations on multiple plots
at once, showing, hiding and more with the click of a button. More here: :ref:`WorkbenchPlotsToolbox`

Plot Window
-----------

.. image:: ../images/Workbench/PlotWindow/PlotWindow.png

This is the window in which all plots will be shown in. There are many things that are possible from this window with
regards to adjusting the the plot, saving/exporting and fitting. This topic really deserves it's own pages and thus
further descriptions can be found here: :ref:`WorkbenchPlotWindow`

