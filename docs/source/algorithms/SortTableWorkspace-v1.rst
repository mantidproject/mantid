
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm sorts rows in a table workspace. Give the names of the columns to sort by in Columns property. The Ascending property allows one to set the sorting order for each column. If a single value is set to Ascending then it applies to all columns.

When multiple columns are given the table is sorted by the first name in the list. If this column contains equal values then these rows are sorted by the second column and so on.


Usage
-----

**Example - SortTableWorkspace**

.. testcode:: SortTableWorkspaceExample

    # a method to print a row
    def print_row(row):
        print("{{'y': '{0}', 'x': {1}, 'z': {2}}}".format(row["y"],row["x"],row["z"]))

    # Create a host workspace
    table = CreateEmptyTableWorkspace()
    table.addColumn("int", "x")
    table.addColumn("str", "y")
    table.addColumn("double", "z")

    # Put in some values
    table.addRow([3, "three (3)", 0.0])
    table.addRow([1, "one (3)", 1.0])
    table.addRow([1, "one (2)",2.0])
    table.addRow([2, "two (1)", 3.0])
    table.addRow([3, "three (2)", 4.0])
    table.addRow([3, "three (2)", 5.0])
    table.addRow([2, "two (2)", 6.0])
    table.addRow([1, "one (1)", 7.0])
    table.addRow([2, "two (1)", 8.0])
    table.addRow([2, "two (2)",9.0])

    # Sort in ascending order
    sorted_asc = SortTableWorkspace(table, Columns = ['x','y','z'])
    print('Sorted ascending')
    for i in range(sorted_asc.rowCount()):
        print_row(sorted_asc.row(i))

    # Sort in descending order
    sorted_des = SortTableWorkspace(table, Columns = ['x','y','z'], Ascending = [False])
    print('Sorted descending')
    for i in range(sorted_des.rowCount()):
        print_row(sorted_des.row(i))


Output:

.. testoutput:: SortTableWorkspaceExample

    Sorted ascending
    {'y': 'one (1)', 'x': 1, 'z': 7.0}
    {'y': 'one (2)', 'x': 1, 'z': 2.0}
    {'y': 'one (3)', 'x': 1, 'z': 1.0}
    {'y': 'two (1)', 'x': 2, 'z': 3.0}
    {'y': 'two (1)', 'x': 2, 'z': 8.0}
    {'y': 'two (2)', 'x': 2, 'z': 6.0}
    {'y': 'two (2)', 'x': 2, 'z': 9.0}
    {'y': 'three (2)', 'x': 3, 'z': 4.0}
    {'y': 'three (2)', 'x': 3, 'z': 5.0}
    {'y': 'three (3)', 'x': 3, 'z': 0.0}
    Sorted descending
    {'y': 'three (3)', 'x': 3, 'z': 0.0}
    {'y': 'three (2)', 'x': 3, 'z': 5.0}
    {'y': 'three (2)', 'x': 3, 'z': 4.0}
    {'y': 'two (2)', 'x': 2, 'z': 9.0}
    {'y': 'two (2)', 'x': 2, 'z': 6.0}
    {'y': 'two (1)', 'x': 2, 'z': 8.0}
    {'y': 'two (1)', 'x': 2, 'z': 3.0}
    {'y': 'one (3)', 'x': 1, 'z': 1.0}
    {'y': 'one (2)', 'x': 1, 'z': 2.0}
    {'y': 'one (1)', 'x': 1, 'z': 7.0}

.. categories::

.. sourcelink::
