.. _AddSpinBox:

==============
Add a Spin Box
==============

A spin box is a useful way to ensure that the user only selects
integer values. The user can type an integer into the spin box or
increment the value using the arrows. It is also possible to place
constraints on the spin box such as a maximum value.

.. code-block:: python

    self._spin = QSpinBox()
    grid.addWidget(self._spin)

To add a spin box to the GUI the above code needs to be added to the
``__init__`` function of the view.
