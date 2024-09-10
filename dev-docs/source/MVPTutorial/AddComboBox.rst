.. _AddComboBox:

==============
Add a ComboBox
==============

A ComboBox is useful when there is a finite list of options that the
user can choose from. If a line edit was used instead then a typo
would prevent the GUI from working correctly; a ComboBox prevents this
possibility.

The following code should be added to the view's ``__init__`` function.

.. code-block:: python

    self._combo = QComboBox()
    options = ["one", "two", "three"]
    self._combo.addItems(options)
    grid.addWidget(self._combo)

The first line creates a ComboBox widget. The second line defines the
options that will be displayed within the ComboBox and the third line
adds the options to the ComboBox. The last line adds it to the layout.
