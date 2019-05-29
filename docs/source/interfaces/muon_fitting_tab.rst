.. _muon_fitting_tab-ref:

Fitting Tab
-----------

.. image::  ../images/muon_interface_tab_fitting.png
   :align: right
   :height: 500px

The tools in this tab are built to assist in fitting functions to data from muon instruments.

**Fit** Fits the functions in the function browser to the selected group(s)

**Single Fit** Choose this option to run the fitting tool for only a single group

**Sequential Fit** Choose this option to run the fitting tool for several groups sequentially

**Simultaneous Fit** Choose this option to run the fitting tool for several groups simultaneously

**Select data to fit** this opens up the dialog box to choose which of the groups in the currently loaded run(s) you
want to fit to.

**Display parameters for** When fitting data to multiple workspaces use this to switch between different selected
workspaces and manipulate initial values for fit functions separately.

Fit function browser
^^^^^^^^^^^^^^^^^^^^

Right clicking in the central box will bring up a context menu allowing you to add or remove functions or backgrounds.
Once functions or backgrounds are added to the browser you can use to modify the initial guess for a varabile and set it
to be shared between all other functions in this fit with the global check box.
All selected workspaces will share the same composite fitting function but can have different variables.

Fit properties browser
^^^^^^^^^^^^^^^^^^^^^^

**Time Start/End** The values here set the boundaries that the fitting function will be calculated between.

**Minimizer** Choose the technique to minimize the fitness function.

**Fit To Raw Data** If this is checked it will use the raw data from the instrument with its default bins, if it is
unchecked it will use data rebinned using the specifications from the home tab.

**Evaluate Function As** Using ths you can choose weather the fitting function will treat the function as a histogram or
a CenterPoint.

Used By
^^^^^^^

:ref:`Muon Analysis 2 <MuonAnalysis_2-ref>`