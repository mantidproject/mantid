.. _Fit_Script_Generator-ref:

Fit Script Generator
====================

.. image::  ../../images/FitScriptGenerator.PNG
   :align: right
   :height: 400px

.. contents:: Table of Contents
  :local:

Interface Overview
------------------

This interface is used to generate a python script used for sequential or simultaneous fitting. The interface allows you to select
specific domains of a workspace, and choose different fit ranges for each domain. For simultaneous fitting, the interface allows
you to choose different fit functions for different domains. It also allows great flexibility when setting a Global tie. See the
usage examples below for a full explanation of how to use this interface.

Interface Options
-----------------

**Remove Domains**
Removes the selected domains from the domain data table.

**Add Domains**
Choose workspace domains to load into the domain data table.

**Domain Data Table**
Displays the loaded workspace domain data and their corresponding fit range. The fit ranges can be different for each domain, and
can be edited by double clicking the ``StartX`` or ``EndX`` cell. The Function Index corresponding to each domain is displayed when
Simultaneous fitting mode is selected.

**Apply function changes to**
There are several changes you can make to the selected fit function such as adding or removing a fit function, changing the value of
a parameter, tying a parameter or constraining a parameter. These changes can be applied to all the domains in the ``Domain Data Table``
if ``All Domains`` is selected, or only to the domains with highlighted rows if ``Selected Domains`` is selected.

**Function Browser**
The Function Browser allows you to add or remove fit functions and adjust their parameters and attributes. It also allows you to add
Fixes, Ties and Constraints. The Function Index corresponding to a specific domain is displayed at the top of the Function Browser
when in Simultaneous fitting mode.

**Fitting Mode**
Determines whether to generate a python script for Sequential or Simultaneous fitting.

**Generate Script to File**
The data selected in the Fit Script Generator is used to generate a python script, which is then saved to a file.

**Generate Script to Clipboard**
The data selected in the Fit Script Generator is used to generate a python script, which is then copied to the clipboard. Paste this
into a python script editor window.

Concepts
--------

**Domain**
This refers to a set of X, Y and error data at a specific index of a :ref:`MatrixWorkspace <MatrixWorkspace>`.

**Function Index**
Different domains can have the same function, and therefore have identically named parameters. A Function Index is used to differentiate
between these identically named parameters by specifying the exact location of a parameter. For example if you are performing a simultaneous
fit with two domains each containing a :ref:`FlatBackground <func-FlatBackground>`, the first domain will have a Function Index of `f0.`
while the second domain has a Function Index of `f1.`. There are therefore two parameters, `f0.A0` and `f1.A0`.

**Attribute**
An attribute is a property of a function which doesn't change during fitting.

**Parameter**
A parameter is a decimal value property of a function which will change during fitting. It has an error.

**Fix**
A Fix is where a parameters value is forced to stay the same during a fit. It is a type of Tie.

**Constraint**
A Constraint is where a parameters value is forced to stay within a defined region during a fit.

**Tie**
A Tie is where a parameters value is set to be equal to the value of another parameter, or an expression, during a fit.

Sequential Fitting Concepts
---------------------------

During a Sequential fit, each domain is fitted one after another. The parameter values at the end of a domain fit are
passed to the next domain fit to be used as the initial parameter values.

**Local Tie**
A local tie is when you tie a parameter to another parameter which is in the same domain. This can be done for both
sequential and simultaneous fitting.

Simultaneous Fitting Concepts
-----------------------------

During a Simultaneous fit, each domain is fitted concurrently. This allows you to tie a parameter to another parameter
in a different domain.

**Global Tie**
A global tie is when you tie a parameter to another parameter which is in a different domain. This can only be done for
simultaneous fitting.

**Global Parameter**
A global parameter is a type of global tie. This is where each domain contains the same parameter, and it is 'shared' by
each domain during a simultaneous fit.

Usage Example setup
-------------------

The following usage examples require some initial setup:

1. Download the ISIS Sample data set, `available here <http://download.mantidproject.org/>`_.

2. Open the Manage User Directories dialog. Add the location of the ISIS Sample data set to the search path.

Usage Example for Sequential Fitting
------------------------------------

1. Run the following script to load some Indirect Inelastic reduced data

.. code-block:: python

    from mantid.simpleapi import *

    Load(Filename=r'irs26176_graphite002_red.nxs', OutputWorkspace='irs26176_red')

2. Open the Fit Script Generator interface.

3. Click ``Add Domains`` and select the 'irs26176_red' workspace.

4. In the ``Workspace Indices`` field enter '0-3'.

5. Click ``Add`` and the ``Close``.

6. Right click on the ``Function Browser`` and add a ``Lorentzian`` function.

7. Add a second ``Lorentzian`` function in the same way.

8. Change the ``Amplitude`` parameter values to 1.5

9. Make sure the ``Fitting Mode`` is Sequential

10. Click ``Generate Script to Clipboard``, and then paste into an empty python script window.

11. Run the script and you will see the results of a sequential fit.

.. image::  ../../images/FitScriptGenerator_SequentialFit.png
   :align: center
   :height: 300px

Usage Example for Simultaneous Fitting
--------------------------------------

1. Run the following script to load some Indirect Inelastic reduced data. This will add a background to the first spectrum.

.. code-block:: python

    import numpy as np
    from mantid.simpleapi import *

    irs26176_red = Load(Filename=r'irs26176_graphite002_red.nxs')

    # Create a background workspace
    y_values = np.zeros(len(irs26176_red.dataY(0)) * irs26176_red.getNumberHistograms())
    y_values[:len(irs26176_red.dataY(0))] = 1.0
    background = CreateWorkspace(DataX=irs26176_red.dataX(0), DataY=y_values, NSpec=irs26176_red.getNumberHistograms(), UnitX='DeltaE', Distribution=True, ParentWorkspace='irs26176_red')

    # Add the background
    Plus(LHSWorkspace=irs26176_red, RHSWorkspace='background', OutputWorkspace='irs26176_red')

1. Open the Fit Script Generator interface.

2. Click ``Add Domains`` and select the 'irs26176_red' workspace.

3. In the ``Workspace Indices`` field enter '0-3'.

4. Click ``Add`` and the ``Close``.

5. Change the ``Fitting Mode`` to Simultaneous.

6. Change ``All Domains`` to ``Selected Domains``, and then select the top domain in the table.

7. Right click on the Function Browser and add a ``FlatBackground``. This will only add this function to the selected domain.
   Selecting the other table rows will show they do not have any fit functions yet.

8. Change the 'A0' parameter in the ``FlatBackground`` to a value of 1.0, and 'Fix' it by right clicking on the parameter.

9. Change ``Selected Domains`` back to ``All Domains``.

10. Right click on the Function Browser and add a ``Lorentzian``. This will add the ``Lorentzian`` function to all of the domains.

11. Add another ``Lorentzian`` so there are two in each of the domains.

12. Change the ``Amplitude`` parameter values in each of the domains to 1.5.

13. Select any table row that isn't the first domain table row.

14. Select the first Peak Centre parameter, right click and add a tie to ``f0.f1.PeakCentre``. This is a global tie.

15. Click ``Generate Script to Clipboard``, and then paste into an empty python script window.

16. Run the script and you will see the results of a simultaneous fit. Notice the background in the first spectrum has been accounted for.

.. image::  ../../images/FitScriptGenerator_SimultaneousFit.png
   :align: center
   :height: 300px

.. categories:: Interfaces General
