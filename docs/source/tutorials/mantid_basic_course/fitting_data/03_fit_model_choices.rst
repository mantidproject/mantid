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
background. Just follow the steps described earlier and add more peaks
to the model. The peaks don't have to be of the same type.

.. figure:: /images/FittingMultiplePeaks.png
   :alt: FittingMultiplePeaks.png
   :width: 900px


Non peak model + background
===========================

Fitting is not limited to peaks. You can
select any other function from our list via right-click "Add other
function".

.. figure:: /images/AddOtherFunctionOptionEMU.png
   :alt: AddOtherFunctionOption.png
   :width: 400px


Custom fitting function
=======================

User defined function
---------------------

You may have spotted "UserFunction". It can be chosen by right-clicking on a plot and selecting "Add other function". It accepts a "Formula" as text string of a mathematical formula. All variables in the formula are treated as
parameters, except for "x" which is the argument.

.. figure:: /images/AddedUserFunction.png
   :alt: AddedUserFunction.png
   :width: 350px

The formula can simply be entered into Fit Property
Browser or with the help of the User Function Dialog:

.. figure:: /images/UserFunctionDialog.png
   :alt: UserFunctionDialog.png
   :width: 500px

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
   :width: 600px

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

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Complex_Case|Mantid_Basic_Course|MBC Intelligent Fitting }}

.. |AddButton.png| image:: /images/AddButton.png
.. |UseButton.png| image:: /images/UseButton.png
.. |SaveFunctionButton.png| image:: /images/SaveFunctionButton.png
.. |RemoveButton.png| image:: /images/RemoveButton.png
