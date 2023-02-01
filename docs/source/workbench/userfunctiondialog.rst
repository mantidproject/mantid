====================
User Function Dialog
====================

The User Function dialog helps in editing the ``Formula`` attribute of
:ref:`UserFunction <func-UserFunction>`.
It offers a list of functions that can be used, helps to resolve parameter name clashes,
allows to save frequently used formulas.

.. image:: /images/Workbench/UserFunctionDialog/UserFunctionDialog_main.png
   :scale: 70%
   :align: center

The upper half of the dialog provides access to the built-in and saved custom functions.
There are three text panels there.
The ``Category`` panel contains a list of categories the functions are divided into.
Clicking on a category shows names of all functions in this category in the next "Functions" panel.
The next panel ``Expression`` displays the mathematical expression for a selected function along with a brief annotation.

The lower half of the dialog contains a multi-line text editor for typing in and editing the formula.
The formula must be a valid muParser expression and a function of single argument x.
Any other variable is treated as a parameter.
The parameters are automatically extracted from the formula and displayed in read-only text field "Parameters".

Button ``Add`` inserts the function selected in Functions list to the formula at the current caret position.
This is most useful to load a previously saved formula.
If a saved formula is used in a larger expression parameter name clashes are possible.
In this case a dialog will pop up offering to rename the parameters of the new expression.
There are three options:

- leave the names as they are,

- add an index or

- rename manually in "New name" column.

The finished formula can be saved for re-use by clicking the ``Save`` button.
A dialog will pop up and offer to select the category, define a name and description for the saved function.
After saving the function will be available for the ``Add`` button.
The ``Remove`` button allows to remove a previously saved formula.
After editing is finished click ``Use`` button to use the expression in UserFunction.
