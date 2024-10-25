==============
Adding a Label
==============

In this section we will add a label to the GUI. A label is a text
display that cannot be edited by the user.

First we need to import the QLabel component from PyQt:

.. code-block:: python

    from qtpy.QtWidgets import QLabel

We then need to add three lines to the view. The first
creates the label widget, the second assigns the label's value (what
it will display) and the last line adds it to the layout of the view.

.. code-block:: python

    self._label = QLabel()
    self._label.setText("Button")
    grid.addWidget(self._label)

This code should be added in the view's constructor (i.e. its
``__init__`` function).
