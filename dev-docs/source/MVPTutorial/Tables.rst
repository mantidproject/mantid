======
Tables
======

Tables are useful way to display a set of related options to a
user. To add a table widget to a GUI the following lines need to be
added to the ``__init__`` function of the view:

.. code-block:: python

    self.table = QtWidgets.QTableWidget(self)
    self.table.setRowCount(2)
    self.table.setColumnCount(2)
    grid.addWidget(self.table)

The first line creates the widget. The second and third lines
determine the size of the table. The fourth line adds it to the
layout.

To add (non-editable) labels to the table the following code is needed:

.. code-block:: python

    text = QtWidgets.QTableWidgetItem(("test"))
    text.setFlags(QtCore.Qt.ItemIsEnabled)
    row = 0
    col = 0
    self.table.setItem(row, col, text)

    row = 1
    text2 = QtWidgets.QTableWidgetItem(("another test"))
    text2.setFlags(QtCore.Qt.ItemIsEnabled)
    self.table.setItem(row, col, text2)

    row = 0
    col = 1
    self.table.setCellWidget(row, col, self.combo)
    row = 1
    self.table.setCellWidget(row, col, self.spin)

The first line creates a widget with the label ``test`` and the second
flag ensures that a user cannot edit the value. The label is added to
the table with the ``setItem`` function.

A useful feature of tables is that they can contain a widget within
one of the cells. The last five lines of the above code adds a
ComboBox and spin box to the table. It is important to note that the
widgets will now only appear within the table.
