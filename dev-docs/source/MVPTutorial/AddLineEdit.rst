==============
Add a LineEdit
==============

Sometimes it is necessary for the user to input some information. The
most versatile way of allowing this is to use a line edit, which
allows a user to enter arbitrary text. Adding the following code to
the ``__init__`` function of the view will add a line edit:

.. code-block:: python

   self._line_edit = QLineEdit()
   grid.addWidget(self._line_edit)

It is possible to have a default value within the line edit. It is
also possible to make it impossible for the user to modify the line
edit (useful for tables).

Care should be taken before using a line edit as it can give a user
too much freedom. If you know that the input is an integer then a
:ref:`spin box <AddSpinBox>` is better. If there is a finite list of
possible options then a :ref:`combo box <AddComboBox>` would be a
better choice.
