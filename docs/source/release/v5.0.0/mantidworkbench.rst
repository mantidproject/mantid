=======================
MantidWorkbench Changes
=======================

.. contents:: Table of Contents
   :local:

New
###
- Waterfall Plots
- Mantid Workbench can now load all of the workspaces from projects saved from Mantidplot.  Graphs and interface values are not imported from the project.
- Added a compatibility implementation of plotSpectrum and plotBin for python scripts in Workbench, all of the previous arguments are supported with the exception of distributionwhich will be ignored. For example the following commands are supported.

.. code-block:: python

	#setup a couple of workspaces
	ws = CreateSampleWorkspace(Function='Powder Diffraction')
	ws2 = ws+ws

	#simple plots
	plotSpectrum(ws ,1)
	plotBin(ws ,1)

	#With Error Bars
	plotSpectrum(ws ,1,error_bars=True)
	plotBin(ws ,1,error_bars=True)

	#Multi line plots
	plotSpectrum([ws ,ws2],1)
	plotBin([ws ,ws2],1)
	plotSpectrum([ws ,ws2],[1,2,3])
	plotBin([ws ,ws2],[1,2,3])
	plotSpectrum(["ws ","ws2"],[1,2,3])

	#plotting with spectra numbers
	plotSpectrum(ws,spectrum_nums=2,error_bars=True)
	plotSpectrum(ws,spectrum_nums=[2,4,6])
	plotSpectrum([ws,ws2],spectrum_nums=list(range(2,10)))

	# add a curve to an existing plot
	plot1 = plotSpectrum(ws ,1,error_bars=True)
	plot1 = plotSpectrum(ws ,error_bars=True,window=plot1)

	# clear an existing plot use that window to plot
	plot2 = plotSpectrum(ws ,1,error_bars=True)
	plot2 = plotSpectrum(ws2,1,error_bars=True,window=plot2, clearWindow = True)

	# plot as points not lines
	plotSpectrum(ws ,1,error_bars=True,type=1)
	plotBin(ws ,1,type=1)

	# plot as waterfall graphs
	plotSpectrum(["ws","ws2"],[1,2,3],waterfall=True)


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
- We have limited the maximum rate of algorithm progress updates to the progress bar to 1000/second.  This has resulted in a workbench completing certain intensive python scripts 4 times faster.
- Algorithm dialogs will now use the selected workspace as the InputWorkspace when running an algorithm from the algorithms toolbox, as MantidPlot did.
- Toggle Whitespace in the editor now shows line endings as well as spaces and tabs
- The Save menu action in the workspaces toolbox to save using version 1 of the SaveAscii algorithm has been removed as no one was using it and it only added confusion. The option to save using the most recent version of SaveASCII is still available.
- You can now search for functions when doing fits.
- A help button has been added to the fitting add function dialog.
- The progress reporting for scripts has been vastly improved and now reports at the line level.
- Warnings from the Python ``warnings`` module are now show as warnings and not errors in the log display.

Bugfixes
########
- Fixed an issue with Workspace History where unrolling consecutive workflow algorithms would result in only one of the algorithms being unrolled.
- Workbench now saves python files properly on windows and does not double up on line feed characters.
- Fixed a couple of errors in the python scripts generated from plots for newer versions of Matplotlib.
- Colorbar scale no longer vanish on colorfill plots with a logarithmic scale
- Figure options no longer causes a crash when using 2d plots created from a script.
- You can now execute algorithms with multiple versions by double clicking on them in the algorithm toolbox, this will now execute them rather than opening the tree to show previous versions.  You can still click on the arrow to see and execute previous versions.
- Running an algorithm that reduces the number of spectra on an active plot (eg SumSpectra) no longer causes an error
- Fix crash when loading a script with syntax errors
- The Show Instruments right click menu option is now disabled for workspaces that have had their spectrum axis converted to another axis using :ref:`ConvertSpectrumAxis <algm-ConvertSpectrumAxis>`.  Once this axis has been converetd the workspace loses it's link between the data values and the detectors they were recorded on so we cannot display it in the instrument view.
- MonitorLiveData now appears promptly in the algorithm details window, allowing live data sessions to be cancelled.
- Figure options on bin plots open without throwing an error.
- The help button in fitting now finds the relevant page.
- Fixed an issue with changing normalisation on single spectra plots done from a script
- Fixed an issue where fitting a distribution workspace was normalised twice.
- Overplots will be normalized by bin width if they are overplotting a curve from a workspace which is a distribution.
- Several bugs in the way Python scripts were parsed and executed, including blank lines after a colon and tabs in strings, have been fixed.
- Axes limits of a plot no longer automatically rescale when errorbars are on/off 
- Axes editor menu now reads the grid visibility of the plot (i.e. the grid checkbox will always show the current grid visibility when an axis is double clicked)
- Fixed a crash in the TOFConverter interface when leaving input fields blank or using invalid characters. 
- Fixed bug that caused an error if a MDHistoWorkspace was plotted and a user attempted to open a context menu.
- Fixed a bug which caused graphic scaling issues when the double-click menu was used to set an axis as log-scaled.
- Fixed a bug where the colorbar in the instrument view would sometimes have no markers if the scale was set to SymmetricLog10.

:ref:`Release 5.0.0 <v5.0.0>`
