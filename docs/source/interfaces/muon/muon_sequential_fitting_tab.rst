.. _muon_sequential_fitting_tab-ref:

Sequential Fitting Tab
------------------------

.. image::  ../../images/muon_interface_tab_sequential_fitting.png
   :align: right
   :height: 400px

This tab allows the user to perform sequential fits. The sequential fits are based on the fit function specified in the fitting tab.
The primary widget in this tab is the Fit Table, which is automatically generated based on the selected Groups/pairs and the fit function defined
in the fitting tab.

Sequential fitting options
^^^^^^^^^^^^^^^^^^^^^^^^^^^
**Sequentially Fit** Sequentiailly fits each of the fit objects specified in the fit table.

**Fit selected** Performs a fit on the fit object selected in the table.

**Plot fit results** Determines whether the results of the fit will be added to the plot window.

**Parameters for previous fit** Uses the parameters from the previous fit calculation as initial values for the next fit. 

**Initial parameters for each fit** Uses the initial values (as specified in the Fit Table) in each fit calculation.

Fit Table
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Each entry of the Fit Table corresponds to a fit object, which comprises a list of input workspaces and a fit function. 

**Run** The runs present in the fit object.

**Groups/pairs** The groups/pairs present in the fit object.

**Fit quality** The quality of the fit returned from the fitting algorithm.

**List of fit parameters** The columns that proceed this correspond to the values of the fit parameters. The value of these
parameters can be edited. 


Used By
^^^^^^^

:ref:`Muon Analysis <MuonAnalysis_2-ref>`
