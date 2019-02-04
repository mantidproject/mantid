==============
Adding a Label
==============

In this section we will add a label to the GUI. A label is a text
display that cannot be edited by the user.

To add a label we need to add three lines to the View. The first
creates the label widget, the second assigns the label's value (what
it will display) and the last line adds it to the layout of the view.

.. code-block:: python

    self.label = QtGui.QLabel()
    self.label.setText("Button")
    grid.addWidget(self.label)

This code should be added in the View's constructor (ie its
``__init__`` function).
