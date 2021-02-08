.. _muon_fitting_tab-ref:

Fitting Tab
-----------

.. image::  ../../images/muon_interface_tab_fitting.png
   :align: right
   :height: 400px

This tab is used to evaluate a single fit object, which corresponds to a list of one or more input workspaces and a fit function.

**Fit** Executes the specified fit.

**Simultaneous Fit** This option will perform a fit for multiple data sets, which may share parameters. 
The simultaneous fit can be performed over a selected run, or selected group/pair. 

**Select Workspace** This drop-down list can be used select the workspace to perform a single fit over.

Fit function browser
^^^^^^^^^^^^^^^^^^^^

.. image::  ../../images/muon_interface_tab_fitting_browser.png
   :height: 125px

Right clicking in the central box will bring up a context menu for adding or removing functions.
The fitting browser here is similar to that used in the :ref:`Multi-dataset Fitting Interface <Multi-dataset-Fitting-Interface>`.

**Property** This is a list of property names, individual functions can be collapsed down for greater legibility.

**Value** This lists the initial guesses for variables within fitting functions. Variables can be fixed for a given fit
from the edit paramiter values dialog box (the ellipsis to the right of the value box).

**Global** If we are performing a simultaneous fit, parameters for each workspace in the fit can be tied by checking this box.

Fit properties browser
^^^^^^^^^^^^^^^^^^^^^^

**Time Start/End** Defines the boundary values for the fit calculation.

**Minimizer** Choose the minimisation method for the fit.

**Fit To Raw Data** If this is checked it will use the raw data for the fit.
If it is unchecked it will use the rebinned data as specified on the home tab.

**Evaluate Function As** Select if to fit to histogram or point data.

**TF Asymmetry Mode** Evaluate the fit in TF asymmetry mode, see :ref:`Calculate Muon Asymmetry <algm-CalculateMuonAsymmetry>`. Note that currently the TF asymmetry parameters will not automatically update when a new run is loaded.

Used By
^^^^^^^

:ref:`Muon Analysis <MuonAnalysis_2-ref>`
:ref:`Frequency Domain Analysis <Frequency_Domain_Analysis_2-ref>`
