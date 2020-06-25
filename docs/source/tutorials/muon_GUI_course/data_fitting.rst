.. _data_fitting:

============
Data Fitting
============

.. contents:: Table of Contents
  :local:

Fit Function
============

There are alternate ways, within Mantid, in which data can be fitted with a function, other than using the Muon Analysis GUI; 
one such solution is the Fit Function tool. The Fit Function tool is similar to the Data Analysis section of the Muon Analysis GUI, 
however, it is not restricted to muon analysis, and thus contains many more built-in functions.

To demonstrate the process of fitting a function to a workspace with the Fit Function tool, follow the instructions below.

1.  Open the file HIFI00062798.nxs using  the Muon analysis GUI, this can then be closed, as it won't be used for data analysis.
2.  Plot the long pair from HIFI62798 Pairs (plotting spectrums described in `03_displaying_1D_data`_).
3.  Click on the data plot window and click the Fit button next to the gear icon. See Figure 36.

.. figure:: /images/Fit_Button2.PNG
    :align: center

    Figure 36: The Fit Function button and the interface.

4.  A new panel should now open on the left hand side of the window, this is the Fit Function panel.
5.  Note the vertical green limit selection lines present on the plot - by clicking and dragging these the data to be fitted can be constrained; 
    try this now to set the fit limits to ~0 and ~12 :math:`{\mu s}`. The lines are not currently present in the Fitting tab of the Muon Analysis interface. Alternatively you
    can use the StartX and EndX boxes.
6.  Similar to the Muon Analysis GUI, a function can be added by right clicking the 'Functions' toggle, selecting 'Add Function' and picking a function from the menu. 
    Do this now and add an ExpDecayOsc function to the data.
    
Alternatively, right clicking the plot, with the Fit Function tool open, and selecting 'Add other function...' also adds a function. 
This will bring up a selection box which lists all available fitting functions in alphabetical order.

7.  Fit ExpDecayOsc to the data by selecting Fit in the Fit drop down box. Figure 37 shows the process. 
    Again note that a better fit is achieved if Alpha has been guessed via the Grouping :ref:`tab<the_tabs_grouping>`.

.. figure:: /images/datafittingfig37.gif
    :align: center

    Figure 37: How to fit data to a single workspace, using the Fit function.

User Defined Functions
----------------------

User Defined functions can be added to data using the Fit Function tool, see below for instructions on how to do this.

1.  Load the file EMU00011888.nxs using the Muon Analysis GUI
2.  Click on the arrow next to the EMU11888 workspace in the workspace pane and plot spectrum ending in fwd; Counts as described in 'Loading Data'.
3.  Click the Fit button at the right hand side of the toolbar to open the fit function tool for the data plot. 
4.  Add the 'UserFunction' function, this can be found in the General section of the fit function menu.
5.  In the Functions panel, there will now be an undefined User Function. Expand it to see the parameters and click on the empty box next to the 'Formula' input box, and then the '...' button which appears. 
    This will take you to the User Function dialogue box.
6.  In the large blank white box at the bottom of the window, define the following function; a*exp(-x/b)+c. This is an exponential decay function for our data.
7.  Make sure that the 'Parameters' box contains all of the parameters for the function ('a, b, c') and click 'Use' to define the function.
8.  The three parameters should have now appeared in the function. Change their default values from 0 to 1.
9.  Fit the function to the data. See Figure 38 for the process. 

.. figure:: /images/user_def_fit2.gif
    :align: center

    Figure 38: How to Fit a User Defined function, using the Fit Function tool. 
