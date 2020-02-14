.. _01_fitting_models_to_data:

======================
Fitting Models To Data 
======================

.. raw:: html

    <style> .red {color:#FF0000; font-weight:bold} </style>
    <style> .green {color:#008000; font-weight:bold} </style>    
    <style> .blue {color:#0000FF; font-weight:bold} </style> 
    <style> .orange {color:#FF8C00; font-weight:bold} </style> 

.. role:: red
.. role:: blue
.. role:: green
.. role:: orange

Fitting is the modelling of data where parameters of a model are allowed
to vary during a fitting process until the agreement between model and
data has seen an improvement according to some cost function.

In summary the Mantid fitting provides

-  General fitting capabilities
-  Fitting extras, that make use of data log file and instrument
   geometry information to enhance the user fitting experience
-  Easily expandable

Here we will cover the basic mantid fitting capabilities.

Simple fitting
==============

1. Plot the only spectrum in the data set MUSR00015189_cropped.nxs

|MUSRDataSet.png|

2. Select the Toolbar button to open Fit Property Browser: |PeakFitToolbar.png|

|MUSRDataSetFittingOn.png|

3. Right click on the plot and select "Add other function"

|AddOtherFunctionOption.png|


4. Choose ExpDecay and click OK
|ChooseExpDecay.png|
5. Notice how the ExpDecay function has appeared in the Functions list on the Fit Property Browser, and there are preset Settings below.
For now, just click on the drop-down menu "Fit" and run a normal Fit.
|RunFitOption.png|
6. Examine the results... 

Fit results
===========

After a successful fit the results can be examined in three ways.

A. A **plot of the fitted model** will be added to the graph which now 
   shows the :blue:`Original data`, the :orange:`Calculated fit` and the :green:`Difference between them`. 

B. The Fit Function property browser will show the **fitted parameters**
   instead of their initial values. If you click on the :red:`triangle` beside `fo-ExpDecay` in the Functions list, it will reveal the Output fit parameters (Height and Lifetime values). Also the :red:`Chi-Squared value` is displayed at the top of the Fitting tab.

|MUSRDataSetFittingResults.png|

C. **Output workspaces** will be created and available via the main Mantid Workspaces Toolbox:


.. figure:: /images/FitResults.png
   :alt: FitResults.png
   :width: 600px

   There are three output workspaces:

   1. A TableWorkspace with the name suffixed with "_Parameters". It
   contains the fitting parameters and their corresponding errors.

   .. figure:: /images/ParametersTable.png
      :alt: ParametersTable.png
      :width: 300px

   2. A MatrixWorkspace with the name suffixed with "_Workspace". Its first
   three spectra are: the original data, the calculated model, and the
   difference.

   .. figure:: /images/FitResultWorkspace.png
      :alt: FitResultWorkspace.png
      :width: 350px

   3. Another TableWorkspace with the name suffixed with
   "_NormalisedCovarianceMatrix". It contains the variance-covariance
   matrix normalized to 100.

   .. figure:: /images/CovarianceTable.png
      :alt: Covariance Table
      :width: 300px


.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Exercise_Connecting_Data_To_Instrument|Mantid_Basic_Course|MBC_Complex_Case }}

.. |MUSRDataSet.png| image:: /images/MUSRDataSet.png
   :width: 400px
.. |PeakFitToolbar.png| image:: /images/PeakFitToolbar.png
.. |MUSRDataSetFittingOn.png| image:: /images/MUSRDataSetFittingOn.png
   :width: 500px
.. |AddOtherFunctionOption.png| image:: /images/AddOtherFunctionOption.png
   :width: 500px
.. |ChooseExpDecay.png| image:: /images/ChooseExpDecay.png
.. |RunFitOption.png| image:: /images/RunFitOption.png
.. |MUSRDataSetFittingResults.png| image:: /images/MUSRDataSetFittingResults.png
   :width: 500px

