======
Tables
======

Tables are useful way to display a set of related options to a
user. To add a table widget to a GUI the following lines need to be
added to the ``__init__`` function of the view:

.. code-block:: python

    from qtpy.QWidgets import QTableWidget

    self._table = QTableWidget(self)
    self._table.setRowCount(2)
    self._table.setColumnCount(2)
    grid.addWidget(self._table)

The first line creates the widget. The second and third lines
determine the size of the table. The fourth line adds it to the
layout.

To add (non-editable) labels to the table, we first need the
following imports at the top of the file:

.. code-block:: python

    from qtpy.QtCore.Qt import ItemIsEnabled

We then need to add this code to the ``__init__`` function:

.. code-block:: python

    from qtpy.QWidgets import QTableWidgetItem

    text = QTableWidgetItem(("test"))
    text.setFlags(ItemIsEnabled)
    row = 0
    col = 0
    self._table.setItem(row, col, text)

    row = 1
    text2 = QTableWidgetItem(("another test"))
    text2.setFlags(ItemIsEnabled)
    self._table.setItem(row, col, text2)

    row = 0
    col = 1
    self._table.setCellWidget(row, col, self._combo)
    row = 1
    self._table.setCellWidget(row, col, self._spin)

The first line creates a widget with the label ``test`` and the second
flag ensures that a user cannot edit the value. The label is added to
the table with the ``setItem`` function.

A useful feature of tables is that they can contain a widget within
one of the cells. The last five lines of the above code adds a
ComboBox and spin box to the table. It is important to note that the
widgets will now only appear within the table.

