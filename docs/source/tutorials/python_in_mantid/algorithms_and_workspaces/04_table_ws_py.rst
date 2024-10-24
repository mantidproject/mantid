.. _04_table_ws_py:

===========================
TableWorkspace with Python
===========================


TableWorkspaces are a type of :ref:`workspace <02_ws_types>` designed to handle data other than particle count spectra. They contain a single array, rather than the 3 tables of X-value, Y-value and E-value characteristic of most :ref:`workspaces <03_matrix_ws_py>` and few :ref:`algorithms <Algorithm>` manipulate. TableWorkspaces are analogous to sheets in Microsoft Excel. An example is the :ref:`algm-Fit` algorithm that saves the calculated fit parameters in this format.


.. figure:: /images/ITableWorkspaceHierachy.png
   :alt: ITableWorkspaceHierachy
   :align: center
   :scale: 60%

Creating a TableWorkspace
=========================

The following script creates a TableWorkspace with 2 columns and one row.

.. code-block:: python

    table = CreateEmptyTableWorkspace()
    table.addColumn("double", "x")
    table.addColumn("double", "y")
    table.addRow([1.0, 3.0])

Accessing Data in a TableWorkspace
==================================

There are various ways to access your data from the workspace.

.. code-block:: python

    # Returns a dictionary of values with column names as keys
    print(table.row(0))
    # Returns all the data in the table from the specified column as a list
    print(table.column("y"))
    # Returns just the entry at the specified row and column
    print(table.cell(0,0))

PeaksWorkspace
==============

A peaks workspace is a type of table workspace, with a fixed structure, where each row of the tale represents a peak. This functionality is also exposed to python, however, usage of this type is outside the scope of this course. See :ref:`PeaksWorkspace` for more details.
