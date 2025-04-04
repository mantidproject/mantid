.. _03_fit_model_choices:

=================
Fit Model Choices
=================


Mantid contains an increasing catalog of predefined fitting
function, which can be used to create a model. Some are
demonstrated here.

In addition, it is possible to create new fitting
functions using the User Function toolbox.


Multiple peaks + background
===========================

It is possible to fit overlapping peaks on a common
background. Load the file `GEM38370_Focussed` and plot spectrum number 6.
Zoom in on the 3 peaks around 2.15 Angstroms. Add a Linear Background, add a Lorentzian peak
to the large central peak, and add a Gaussian peak to both of the smaller peaks, as below.
Run the Fit, and inspect the results.

.. figure:: /images/FittingMultiplePeaks.png
   :alt: FittingMultiplePeaks.png
   :width: 900px
   :align: center


Non peak model + background
===========================

Fitting is not limited to peaks. You can
select any other function from our list via right-click "Add other
function".

Load `EMU00020884` and plot spectrum number 6. Adjust the "EndX" value to 9, and use the Zoom tool to focus on the fitting area. Right-click on the graph and select "Add other function" Type `ExpDecay` and press "OK". Then, edit the parameters of "f0-ExpDecay" by setting Height to 40000 and Lifetime to 1.5. Finally, click "Fit" You should see an exponential decay fit similar to the image below.

.. figure:: /images/AddOtherFunctionOptionEMUDecayOnly.png
   :alt: AddOtherFunctionOption Fit for Only ExpDecay
   :align: center

Next, add the other function : `GausOsc` and run the Fit again. You should find this fit is better.

.. figure:: /images/AddOtherFunctionOptionEMU.png
   :alt: AddOtherFunctionOptionEMU.png
   :align: center


Custom fitting function
=======================

.. _user_defined_function:

User defined function
---------------------

You may have spotted "UserFunction". It can be chosen by right-clicking on a plot and selecting "Add other function". It accepts a "Formula" as text string of a mathematical formula. All variables in the formula are treated as
parameters, except for "x" which is the argument.

.. figure:: /images/AddedUserFunction.png
   :alt: AddedUserFunction.png
   :width: 350px

The formula can simply be entered into Fit Property Browser.
Alternatively, click on the `...` button in the Formula input box
to open the User Function Dialog:

.. figure:: /images/UserFunctionDialog.png
   :alt: UserFunctionDialog.png
   :align: center

Type your function in the large lower text field. Browse the built-in functions above and add (|AddButton.png|) them
to your formula. The fitting parameters are displayed in the Parameters
field (read-only). If the field is empty then your formula contains errors.

You can save the function you have defined, for future use. Click
the Save button |SaveFunctionButton.png| to see the dialog:

.. figure:: /images/SaveUserFunctionDialog.png
   :alt: SaveUserFunctionDialog.png
   :width: 300px

Now your function appears in the list of available functions:

.. figure:: /images/SavedFunctionRecord.png
   :alt: SavedFunctionRecord.png
   :align: center

Any unwanted function can be removed from the list using
|RemoveButton.png| button.

When finished click the Use button |UseButton.png| to insert the formula
into the Fit Function browser.

Tabulated function
------------------

A TabulatedFunction takes its values from a file or a workspace

.. figure:: /images/TabulatedFunction.png
   :alt: TabulatedFunction.png
   :width: 300px

For more information on this, see `here <https://docs.mantidproject.org/nightly/fitting/fitfunctions/TabulatedFunction.html>`_.


.. |AddButton.png| image:: /images/AddButton.png
.. |UseButton.png| image:: /images/UseButton.png
.. |SaveFunctionButton.png| image:: /images/SaveFunctionButton.png
.. |RemoveButton.png| image:: /images/RemoveButton.png
