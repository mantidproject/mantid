=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

New
###
- Waterfall Plots
- Mantid Workbench can now load all of the workspaces from projects saved from Mantidplot.  Graphs and interface values are not imported from the project.

Improvements
############

.. figure:: ../../images/Notification_error.png
   :class: screenshot
   :width: 600px
   :align: right

- If you have ever found it hard to spot when errors appear in the Messages window, and perhaps miss them if there are lots of graphs on the screen, then you will like this.  We have added system notifications when Mantid enounters an error, and directs you to look at the Messages window for details.  You can enable or disable these notifications from the File->Settings window.

.. figure:: ../../images/Notifications_settings.png
   :class: screenshot
   :width: 500px
   :align: left

- You can now save Table Workspaces to Ascii using the `SaveAscii <algm-SaveAscii>` algorithm, and the Ascii Save option on the workspaces toolbox.
- Fit functions can now be put into nested categories and into multiple categories.
- Normalization options have been added to 2d plots and sliceviewer.
- An exclude property has been added to the fit property browser
- The images tab in figure options no longer forces the max value to be greater than the min value.
- The algorithm progress details dialog now fills immediately with all running algorithms rather than waiting for a progress update for the algorithm to appear.
- All the relevant settings from manitdplot have been added to workbench
- Double clicking on a workspace that only has a single bin of data (for example from a constant wavelength source) will now plot that bin, also for single bin workspaces a plot bin option has been added to the right click plot menu of the workspace.
- Default values for algorithm properties now appear as placeholder (greyed-out) text on custm algorithm dialogs.
- The context menu for WorkspaceGroups now contains plotting options so you can plot all of the workspaces in the group.
- Most changes in the settings dialog now take place immediately, no longer needing a restart, such as hiding algorithm categories, interfaces or choosing wether to see invisible workspaces.
- A warning now appears if you attempt to plot more than ten spectra.
- The Save menu action in the workspaces toolbox to save using version 1 of the SaveAscii algorithm has been removed as no one was using it and it only added confusion. The option to save using the most recent version of SaveASCII is still available.

Bugfixes
########
- A few bugs associated with the script editor have been fixed:
  - Previous versions would sometimes error with ```Syntax error: unexpected indent``` with some indented blocks of code, we have improved the sectioning of code ensure that we do not stop a block prematurely.
  - We now only automatically convert tabs to spaces at the start of the line when executing python code (we have to do this otherwise python omplains).  This means that tabs within string literals will remain as tabs.
- Fixed an issue with Workspace History where unrolling consecutive workflow algorithms would result in only one of the algorithms being unrolled.
- Fixed a couple of errors in the python scripts generated from plots for newer versions of Matplotlib.
- Colorbar scale no longer vanish on colorfill plots with a logarithmic scale
- Figure options no longer causes a crash when using 2d plots created from a script.
- Running an algorithm that reduces the number of spectra on an active plot (eg SumSpectra) no longer causes an error
- Fix crash when loading a script with syntax errors
- The Show Instruments right click menu option is now disabled for workspaces that have had their spectrum axis converted to another axis using :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.  Once this axis has been converetd the workspace loses it's link between the data values and the detectors they were recorded on so we cannot display it in the instrument view.
- MonitorLiveData now appears promptly in the algorithm details window, allowing live data sessions to be cancelled.
- Figure options on bin plots open without throwing an error.

:ref:`Release 4.3.0 <v4.3.0>`
