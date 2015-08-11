:orphan:

.. testcode:: mwTest_Python_ITableWorkspace_v2[6]

   table = CreateEmptyTableWorkspace()
   table.addColumn("double", "x")
   table.addColumn("double", "y")
   table.addRow([1.0, 3.0])


.. testsetup:: mwTest_Python_ITableWorkspace_v2[15]

   table = CreateEmptyTableWorkspace()
   table.addColumn("double", "x")
   table.addColumn("double", "y")
   table.addRow([1.0, 3.0])

.. testcode:: mwTest_Python_ITableWorkspace_v2[15]

   # Returns a dictionary of values with column names as keys
   print table.row(0)
   # Returns all the data in the table from the specified column as a list
   print table.column("y")
   # Returns just the entry at the specified row and column
   print table.cell(0,0)

.. testoutput:: mwTest_Python_ITableWorkspace_v2[15]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   {'y': 3.0, 'x': 1.0}
   [3.0]
   1.0


