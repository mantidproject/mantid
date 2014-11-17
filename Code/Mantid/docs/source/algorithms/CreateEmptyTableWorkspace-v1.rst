.. algorithm::

.. summary::

.. alias::

.. properties::

.. _TableWorkspace Python API: http://www.mantidproject.org/Python_ITableWorkspace_v2

Description
-----------

Creates an empty table workspace, which you can then populate with items of
various types.

For a complete list of TableWorkspace methods accessible from Python please see
the `TableWorkspace Python API`_.

Usage
-----

**Example - Creating and Populating a Table**

.. testcode:: ExTable

   my_table = CreateEmptyTableWorkspace()

   my_table.setTitle("My Data List")

   my_table.addColumn("str", "Instrument Name")
   my_table.addColumn("int", "Run Number")

   my_table.addRow(["MUSR", 10245])
   my_table.addRow(["IRIS", 8465])
   my_table.addRow(["SANS2D", 20462])

   print "The run number for IRIS is " + str(my_table.cell("Run Number", 1)) + "."
   print "The number of rows is " + str(my_table.rowCount()) + "."
   print "The title of the table is " + my_table.getTitle() + "."
   print "Remember, the table is a workspace.  It's name is \"" + my_table.getName() + "\"."

Output:

.. testoutput:: ExTable

   The run number for IRIS is 8465.
   The number of rows is 3.
   The title of the table is My Data List.
   Remember, the table is a workspace.  It's name is "my_table".

.. categories::
