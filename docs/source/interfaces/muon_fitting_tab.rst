.. _muon_fitting_tab-ref:

Fitting Tab
-----------

.. image::  ../images/muon_interface_tab_fitting.png
   :align: right
   :height: 500px

This tab is for performing fits on the data.

**Fit** Executes the chosen fit for the fitting functions to the selected data.

**Single Fit** This option performs a fit for a single data set.

**Sequential Fit** This option will perform multiple fits sequentially, using the output of the previous fit as the starting guess. 

**Simultaneous Fit** This option will perform a fit for multiple data sets, which may share parameters. 

Select data to fit
^^^^^^^^^^^^^^^^^^
This launches a dialog box for selecting the data to be used in the fit.


Fit function browser
^^^^^^^^^^^^^^^^^^^^

Right clicking in the central box will bring up a context menu for adding or removing functions.
Once functions or backgrounds are added to the browser you can use to modify the initial guess for a varabile and set it
If the parameter is to be the same for all of the data sets, the global check box should be ticked.
All selected workspaces will share the same composite fitting function but can have different variables.

Fit properties browser
^^^^^^^^^^^^^^^^^^^^^^

**Time Start/End** Defines the boundary values for the fit calculation.

**Minimizer** Choose the minimisation method for the fit.

**Fit To Raw Data** If this is checked it will use the raw data for the fit.
If it is unchecked it will use the rebinned data as specified on the home tab.

**Evaluate Function As** Select if to fit to histogram or point data.

Used By
^^^^^^^

:ref:`Muon Analysis 2 <MuonAnalysis_2-ref>`
