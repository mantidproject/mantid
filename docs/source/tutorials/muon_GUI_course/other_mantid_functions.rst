.. _other_mantid_functions:

==================================================
Other Mantid Functions and Basic Data Manipulation 
==================================================

.. index:: Other Mantid Functions

This section allows the user to:

* familiarise themself with the content of Mantid workspaces
* learn how to read/export data
* learn how to overlay data/change plot style

.. contents:: Table of Contents
  :local:

Workspaces
==========

Workspaces hold the data in Mantid. They come in several forms, but the most common is
the Matrix Workspace which represents XYE data for one or more spectra. In MantidWorkbench the
data contained in a workspace can viewed as a matrix or a table, and graphed in many ways
including Line graphs, and 'colourful' (contour) plots.

Interaction with workspaces is typically through an interface. Matrix Workspaces are
typically created by executing one of Mantid's 'Load' algorithms or are the output of
algorithms which take a Matrix Workspace as input.

In addition to data, workspaces hold a workspace history, which details the algorithms
which have been run on this workspace.

The workspace list pane is found on the left-hand side of the main MantidWorkbench window. 
When a muon data set is loaded via the GUI, the Matrix Workspaces are created automatically and stored within a workspace called **Muon Data**;
in turn this workspace contains the following sub-workspaces:

**NAME12345 MA** which contains all the Muon Analysis data for run 12345 on the NAME instrument. 
This data is further separated into more sub-workspaces, these are listed below.

*   **NAME12345 Raw Data MA**, holds within it raw positron counts for each individual detector.
    To examine the data collected in a single detector, right click on workspace MuonAnalysis and select Plot Spectrum.
    There is one spectrum per detector. In the window that opens up, the number of the spectrum/detector
    to plot, or a range of spectra, can be selected.
*   **NAME12345 Groups MA** contains data in raw counts and asymmetry, collated 
    into the groups specified in the Grouping tab of the interface (see the Grouping section in :ref:`the_tabs` for more). 
    Each workspace's name contains the name of the group it holds data for.
*   **NAME12345 Pairs MA** these workspaces contain the Pairs (such as 'long' which is created by default) specified in the *Grouping Options* tab of the interface (see the Grouping section in :ref:`the_tabs` for more).
    Each workspace's name contains the name of the pair it holds data for. 

**Fitting Results MA** contains the workspaces produced when data is fitted, the spectra and tables within are named with the convention 'NAME12345; Group/Pair Asym; Asymmetry; MA; Fitted; FunctionName'. 
    In order, the parts of these names correspond to: the data run, the group or pair fitted, the fact the data is asymmetry, that it is from Muon Analysis, that it has been 
    fitted, and what function(s) were used. Data in Fitting Results MA is split into the following sub-workspaces:

*   **Fitting Results_workspaces MA** contains the spectra of fitted asymmetry data. These workspaces contain three spectra - the original data, the fit, and the difference between the two, 
    which are assigned spectrum numbers 1, 2, and 3 respectively.
*   **Fitting Results_parameter_tables MA** contains tables of the parameters of the function used for a fit, along with their values and errors for that fit.

In general the muon analysis GUI hides workspaces from the user; however, an
understanding of their functionality is sometimes useful to take advantage of some of the
more advanced features of MantidWorkbench. In particular, for those seeking to develop a
specialist analysis method, the GUI can be used for the initial data reduction, while user
coded Python or C++ algorithms act on workspaces to process the final result. This type
of hybrid workflow demonstrates the versatility of the MantidWorkbench interface.

Other types of object, such as Tables, Matrices and Notes, may be created through the
MantidWorkbench interface. As these are not workspaces the Algorithms can't directly be applied.
However, Tables may be converted to either a Table or Matrix workspace through an
option on the analysis menu for subsequent data processing. For instance, converting the
Results Table to a matrix workspace enables MantidWorkbench fitting the be carried out.

Loading Data
------------

Matrix Workspaces are typically created by executing one of Mantid's 'Load' algorithms or are 
the output of algorithms which take a Matrix Workspace as input. In addition to data, workspaces 
hold a workspace history which lists the algorithms that have been applied to the data.

To load a raw data file *without using* the Mantid Analysis GUI, and examine its content:

    1. In the 'Workspaces' pane, click Load>File
    2. Load the file HIFI00062798.nxs by using the Browse button - See Figure 9(a).

.. figure:: /images/load_file2.gif
    :align: center

    Figure 9(a): Loading the HIFI00062798.nxs data file into Mantid using the Browse button instead of the Analysis GUI.

Workspace Information
---------------------

Click on the arrow beside the file name – this allows information about the format of
data file to be viewed. The following should be seen in the Workspace List pane to
the right of the screen.

It can be seen that a workspace called HIFI00062798 has been created and...

* is a 2D array
* the data has been collected from a silver calibration measurement taken in a transverse field of 20G
* it contains 64 spectra (or histograms i.e. one for each HiFi detector)
* there are 2048 time channels, or bins, per plot

However, the NeXuS format allows a lot more information be stored in a data file than that listed above. As an example 
right click on the file name and select 'Sample Logs'. A list of experiment and
instrument parameters that have been logged during a measurement, from
magnetic fields to sample temperatures, appears.

To interrogate any of these logs double click on the 'Name', try this with Temp_Cryostat as shown in figure 9(b).

.. figure:: /images/sample_logs2.gif
    :align: center

    Figure 9(b): How to open the Sample Logs for the workspace and opening file Temp_Cryostat.

Plotting spectra
----------------

As mentioned, the HIFI00062798.nxs workspace
holds within it raw positron counts / bin for *each* individual HiFi detector. To examine the
data collected in a single detector, right click on HIFI00062798 and select "Plot>Spectrum..." .

On HiFi, as way of example, there are 64 detectors hence ID numbers: 1-64 (1-32 =
upstream detectors, 33-64 = downstream detectors.

Enter a detector (ID) number of choice and click OK to plot the associated raw data. 
This process is illustrated below.

.. figure:: /images/plot_spectrum2.gif
    :align: center

    Figure 9(c): How to plot an individual detector spectrum. This example shows spectrum 10 for the HIFI00062798 dataset.

For information:

* ARGUS has 192 detectors
* MuSR and HIFI have 64 detectors
* EMu has 96 detectors

Exporting Data
==============

To export the data contained within any listed workspace, the Algorithms tab at the 
bottom of the workspace list pane can be used. 

.. figure:: /images/AlgorithmsOptions.PNG
    :align: center

    Figure 10(a): The Algorithms Options

Follow the instructions below to try this

    1. Load the workspace HIFI00062798.nxs, see `Loading Data`_ for more on how to do this.
    2. Using the drop-down menu next to the Execute button, type or select SaveAscii, and click Execute. This is shown in Figure 10(b)

    .. figure:: /images/save_ascii2.gif
        :align: center

        Figure 10(b): Where to find the SaveAscii Algorithm.

    3. The SaveAscii Input Dialog box - shown in Figure should appear. Select a directory (for the written data file) and specify a file name. 
    4. Note the workspace to be exported can be selected from the uppermost dropdown list, next to InputWorkspace, in this case let us leave it as HIFI00062798. 
    5. Define which workspace spectra to export using the WorkspaceIndexMin and WorkspaceIndexMax inputs. Use these to save spectra 10 and 11.
    6. Choose the type of data separator used in the file (CSV is usually a reliable option), add comments and uncheck the WriteXError box.
    7. Export the spectra.

Overlaying and Styling Plots
============================

Overlaying data plots can be useful when trying to compare two different sets of data simultaneously by having them on one individual plot. 
Overlaying data can be done by simply clicking and dragging a workspace onto an existing plot, or can be done via the Overlaying Data option
from the workspace pane.

To try this follow these instructions:

    1. Load the HIFI00062798 workspace and plot spectrum number 10 from the workspace panel, as described in `Loading Data`_ .
    2. Go to the 'Plots' menu by clicking the button of the same name in the bottom left of the window, and ensure that the plot of spectrum 10 is selected in bold.
    3. Return to the workspaces panel and right click on the HIFI00062798 workspace, as before going to Plot but this time selecting 'Overplot Spectrum...' rather than 'Spectrum...'
    4. Spectrum 20 should now be visible on the spectrum 10 plot along with the original data. This process is shown in Figure 11(a) below.

.. figure:: /images/overlay2.gif
    :align: center

    Figure 11(a): How to overlay one plot (detector 20) onto another (detector 10).

Plot Styles
-----------

The plot style a data set can be selected 
using the gear icon at the top of the plot window, this will open the Figure options menu.

    To demonstrate changing a plot's markers and curve colour follow these instructions:

    1. Spectrum 20 of HIFI00062798 should already have been plotted, via the Workspaces pane. If not, do so now.
    2. Click the gear icon above the plot, this will bring up a new Figure options window for the data set.
    3. Go to the Curves tab and use the Color (RGBA) option under Line to change the colour from Blue to Red.
    4. Click Apply to view the changes and save the selected option.
    5. To change the marker style - by default none are shown - use the Style drop-down menu under the Marker heading in bold, use this now to change the marker style to square. Again, click Apply.

    .. figure:: /images/change_style.gif
        :align: center

        Figure 11(b): How to change the line colour and marker style of a plot.

Editing Axes
------------

One can also change the axis settings, such as the maximum and minimum values, and plotting against a logarithmic scale. 
The axis limits can be changed either through the Figure options menu, or by double clicking on the relevant axis, while other .
See the instructions below for an example on how to change the X-Axis limits and set the Y-Scale to logarithmic.

    1. Load the MUSR00024563 dataset using the muon analysis GUI, and plot the backward counts. How to do this is described in the Home section of :ref:`the_tabs`.
    2. Open the Figure options menu as described above, **or** double click on the X axis to Edit axis.
    3. In the Axes tab of the Figure options, set the value in the box labelled Right to 16 **or** do the same for the box labelled Max in the Edit axis dialog.
    4. Press Apply, **or** click the OK button.
    5. In the Y-Axis section of the figure options, use the Scale drop-down menu and change the setting to log **or** double click on the Y axis and tick the box labelled Log.
    6. Click OK and observe the changes to the plot, this process is shown for the Figure options in Figure 11(c), and the Edit axis in 11(d).

    .. figure:: /images/axis_figure_options.gif
        :align: center

        Figure 11(c): Changing the X-Axis scale limits and setting the Y-Axis to logarithmic settings using the Figure options menu.

    .. figure:: /images/edit_axis.gif
        :align: center

        Figure 11(d):  Changing the X-Axis scale limits and setting the Y-Axis to logarithmic using the Edit axis dialog. 
        Note that if the scale limits include negative values when doing this, Mantid 
        will automatically use a 'symmetrical log' scale, which allows for negative values by having a range around 0 where
        the scale is linear not logarithmic.

Fit Function
============

There are alternate ways, within Mantid, in which data can be fitted with a function, other than using the Muon Analysis GUI; 
one such solution is the Fit Function tool. The Fit Function tool is similar to the Data Analysis section of the Muon Analysis GUI, 
however, it is not restricted to muon analysis, and thus contains many more built-in functions.

    To demonstrate the process of fitting a function to a workspace with the Fit Function tool, follow the instructions below.

    1.  Open the file HIFI00062798.nxs using  the Muon analysis GUI, this can then be closed, as it won't be used for data analysis.
    2.  Plot the long pair from HIFI62798 Pairs (plotting spectrums described in `Loading Data`_).
    3.  Click on the data plot window and click the Fit button next to the gear icon. See Figure 12(a).

    .. figure:: /images/Fit_Button2.PNG
        :align: center

        Figure 12(a): The Fit Function button and the interface.

    4.  A new panel should now open on the left hand side of the window, this is the Fit Function panel.
    5.  Note the vertical green limit selection lines present on the plot - by clicking and dragging these the data to be fitted can be constrained; 
        try this now to set the fit limits to ~0 and ~12 :math:`{\mu s}`. The lines are not currently present in the Fitting tab of the Muon Analysis interface
    6.  Similar to the Muon Analysis GUI, a function can be added by right clicking the 'Functions' toggle, selecting 'Add Function' and picking a function from the menu. 
        Do this now and add an ExpDecayOsc function to the data.
    
    Alternatively, right clicking the plot, with the Fit Function tool open, and selecting 'Add other function...' also adds a function. 
    This will bring up a selection box which lists all available fitting functions in alphabetical order.

    7.  Fit ExpDecayOsc to the data by selecting Fit in the Fit drop down box. Figure 12(b) shows the process. 
        Again note that a better fit is achieved if Alpha has been guessed via the Grouping :ref:`tab<the_tabs>`.

    .. figure:: /images/fit_function_tool2.gif
        :align: center

        Figure 12(b): How to fit data to a single workspace, using the Fit function.

User Defined Functions
----------------------

User Defined functions can be added to data using the Fit Function tool, see below for instructions on how to do this.

    1.  Load the file EMU00011888.nxs using the Muon Analysis GUI
    2.  In the Home tab of the GUI, change the Detector Grouping to 'fwd' and the Plot Type to 'Counts', to plot muon counts against time, 
        for the fwd detectors. If this is unclear, re-visit Home in :ref:`the_tabs`. 
        (Without integrated plotting: Go to Muon Data > EMU11888 >EMU11888 Groups and plot the spectrum ending fwd; Counts; #1 as described in `Loading Data`_)
    3.  Click the Fit button at the right hand side of the toolbar to open the fit function tool for the data plot. 
    4.  Add the 'UserFunction' function, this can be found in the General section of the fit function menu.
    5.  In the Functions panel, there will now be an undefined User Function. Expand it to see the parameters and click on the empty box next to the 'Formula' input box, and then the '...' button which appears. 
        This will take you to the User Function dialogue box.
    6.  In the large blank white box at the bottom of the window, define the following function; a*exp(-x/b)+c. This is an exponential decay function for our data.
    7.  Make sure that the 'Parameters' box contains all of the parameters for the function ('a, b, c') and click 'Use' to define the function.
    8.  The three parameters should have now appeared in the function. Change their default values from 0 to 1.
    9.  Fit the function to the data. See Figure 12(c) for the process. 

.. figure:: /images/user_def_fit2.gif
        :align: center

        Figure 12(c): How to Fit a User Defined function, using the Fit Function tool. 




