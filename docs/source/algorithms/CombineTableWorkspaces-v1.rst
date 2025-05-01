.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to combine a pair of :ref:`Table Workspaces <Table Workspaces>`
into a single table. The current algorithm is very lightweight and is intended for situations
where multiple tables are produced by the same processing pipeline, but ultimately the output
of all the generated data in a single table would suffice. As such, the algorithm requires that
the Column names match exactly, are in the exact same order and that corresponding columns have
identical data types.

The currently supported data types are ``double``, ``int``, ``bool``, ``float``, ``string``,
``size_t``, and ``V3D``.

Example Usage
-------------

Here we have a python function to generate us some example table workspaces:

.. code:: python

    def create_example_table(table_name, data_types, column_names, n_rows, data):
        """
        table_name: name for output workspace
        data_types: collection of strings of data types, one for each column
        column_names: collection of strings of column names, one for each column
        n_rows: number of rows in table
        data: collection of collection of values for each row, if only one is specified it will be
              reused for all rows
        """
        table = CreateEmptyTableWorkspace(OutputWorkspace = table_name)
        for i, dt in enumerate(data_types):
            table.addColumn(dt, column_names[i])
        for r in range(n_rows):
            if len(data) == 1:
                table.addRow(data[0])
            else:
                table.addRow(data[r])

    # here is an example table with two columns, labelled 1 and 2, with 2 rows each, both taking integer values
    create_example_table("test_table1", ["int", "int"], ["1", "2"], 2, ([0,1],[2,3]))

We can use this helper function to demonstrate the algorithm usage:

.. code:: python

    create_example_table("test_table1", ["int", "int"], ["1", "2"], 2, ([0,1],[2,3]))
    create_example_table("test_table2", ["int", "int"], ["1", "2"], 2, ([4,5],[6,7]))

    CombineTableWorkspaces("test_table1", "test_table2", OutputWorkspace = "comb1")

This also works for tables with mixed data types

.. code:: python

    create_example_table("test_table3", ["str", "bool", "int"], ["1", "2", "3"],
                     50, (["a", True, 1],))

    CombineTableWorkspaces("test_table3", "test_table3", OutputWorkspace = "comb2")

.. categories::

.. sourcelink::
