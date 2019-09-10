.. _02_fit_model_choices:

=================
Fit Model Choices 
=================

Mantid contains a constantly increasing catalog of predefined fitting
function which can be used to create a model. Some of these will be
demonstrated here. In addition, it is possible to create new fitting
functions using the Fit Function toolbox, or by extending Mantid using
a plugin mechanism. The former will be covered here.

Multiple peaks + background
===========================

Mantid provides the ability to fit overlapping peaks on a common
background. Just follow the steps described earlier to add more peaks
to the model. The peaks don't have to be of the same type.

.. figure:: /images/FittingMultiplePeaks.png
   :alt: FittingMultiplePeaks.png
   :width: 500px

Non peak model + background
===========================

The Mantid fitting tool isn't limited to peaks and backgrounds. You can
select any other function from a list offered by the "Add other
function.." option.

.. figure:: /images/AddOtherFunctionOption.png
   :alt: AddOtherFunctionOption.png
   :width: 400px

Use custom fitting function
===========================

User defined function
---------------------

Mantid provides a user defined function called UserFunction. It has an
attribute called "Formula" which accepts a text string with a
mathematical formula. All variables in the formula are treated as
parameters except for "x" which is the argument.

.. figure:: /images/AddedUserFunction.png
   :alt: AddedUserFunction.png
   :width: 200px

The formula can either be entered in the text editor in the Fit Function
browser or constructed with the help of the User Function Dialog.

.. figure:: /images/UserFunctionDialog.png
   :alt: UserFunctionDialog.png
   :width: 500px

Edit your function in the text field, browse and add (|AddButton.png|)
to your formula any built-in or saved function. The fitting parameters
are extracted automatically and displayed in the Parameters read-only
field. If the field is empty then your formula contains errors.

When finished click the Use button |UseButton.png| to insert the formula
into the Fit Function browser.

The constructed formula can be saved permanently for future use. Click
the Save button |SaveButton.png| to see the dialog:

.. figure:: /images/SaveUserFunctionDialog.png
   :alt: SaveUserFunctionDialog.png
   :width: 200px

Now your function appears in the list of available functions:

.. figure:: /images/SavedFunctionRecord.png
   :alt: SavedFunctionRecord.png
   :width: 500px

Any unwanted function can be removed from the list using
|RemoveButton.png| button.

Tabulated function
------------------

A TabulatedFunction takes its values from a file or a workspace

.. figure:: /images/TabulatedFunction.png
   :alt: TabulatedFunction.png
   :width: 200px

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Setup_And_Execute_First_Fit|Mantid_Basic_Course|MBC Intelligent Fitting }}

.. |AddButton.png| image:: /images/AddButton.png
.. |UseButton.png| image:: /images/UseButton.png
.. |SaveButton.png| image:: /images/SaveButton.png
.. |RemoveButton.png| image:: /images/RemoveButton.png
