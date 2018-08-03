.. |load_symbol| image:: /images/LoadFileToolbar.png

.. _loading and displaying:
                 
===========================
Loading and displaying data
===========================

Loading data
============

There are a number of options for loading up data in MantidPlot. The most convenient
approach is to use the “Load” option in the "Workspaces" window, by default the
"Workspaces" window is on the right hand side of the GUI.

* Click on the the "Load" button and select "File". This will open the Load Dialog window.
* Browse to the location of the file.
* If you have added the directory containing this file to your data search directories 
  (see :ref:`getting started`) you can simply enter the filename in the File textbox.
* Mantid will suggest the OutputWorkspace name, but you can change this if desired.
* Press "Run". A Workspace will appear in the "Workspaces" window.

Other ways to load data are:

* Navigating to the "File->Load->File" menu item.
* Clicking the |load_symbol| symbol in the toolbar.
* Using the "Load" algorithm. (:ref:`load algorithm`).

Types of Data Files
###################

Mantid can load many different data formats. A few important examples are:

* ISIS, SNS, ILL, PSI .nexus data files.
* ISIS .raw data and log files.
* Simulated data formats.
* Ascii data, Table data, etc.
* Live data streams.

The "Load" feature can handle any of these formats automatically.

Multiple Data Files
###################

You can load multiple files into mantid with a single Load command, either keeping each workspace separate, 
or summing the data into a single workspace:

* <name>, <name> : load a list
* <name>+<name>: sum the data in the files
* <name>:<name>: load a range of files
* <name>-<name>: sum a range of files

