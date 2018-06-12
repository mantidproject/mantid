.. _Table Workspaces:

================
Table Workspaces
================

.. contents::
  :local:

Overview
--------

Table workspaces are general purpose workspaces for storing data of
mixed types. A table workspace is organized in columns. Each column has
a name and a type - the type of the data in that column. Data can be accessed by column, by row, or by cell


Working with Table Workspaces in Python
---------------------------------------

For full details of the Table Workspaces python type itself please see :py:obj:`this page <mantid.api.ITableWorkspace>`.

Accessing Workspaces
####################

The methods for getting a variable to an Table Workspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is an Table Workspace you can use this:

.. testcode:: CheckTableWorkspace

    from mantid.api import ITableWorkspace

    tableWS = CreateEmptyTableWorkspace()

    if tableWS is ITableWorkspace:
        print(tableWS.getName() + " is a " + tableWS.id())

Output:

.. testoutput:: CheckEventWorkspace
    :options: +NORMALIZE_WHITESPACE

    tableWS is a TableWorkspace

Creating a Table Workspace in Python
####################################

Most of the time Table workspaces are the output of certain algorithms, but you can create them yourself should you wish.

.. testcode:: CreateTableWorkspace

    from mantid.kernel import V3D

    # Create PositionTable
    tableWS = CreateEmptyTableWorkspace()

    # Add some columns, Recognized types are: int,float,double,bool,str,V3D,long64
    tableWS.addColumn(type="int",name="Detector ID")  
    tableWS.addColumn(type="str",name="Detector Name")  
    tableWS.addColumn(type="V3D",name="Detector Position")

    # Populate the columns for three detectors
    detIDList = range(1,4)
    detPosList = [ V3D(9.0,0.0,0.0), V3D(10.0,3.0,0.0), V3D(12.0,3.0,6.0)]
    for j in range(len(detIDList)):
        nextRow = { 'Detector ID': detIDList[j], 
                    'Detector Name': "Detector {0}".format(detIDList[j]),
                    'Detector Position': detPosList[j] }
        tableWS.addRow ( nextRow )

Table Workspace Properties
##########################

.. testsetup:: TableWorspaceProperties

    from mantid.kernel import V3D

    # Create PositionTable
    tableWS = CreateEmptyTableWorkspace()

    # Add some columns
    tableWS.addColumn(type="int",name="Detector ID")  
    tableWS.addColumn(type="str",name="Detector Name")  
    tableWS.addColumn(type="V3D",name="Detector Position")

    # Populate the columns for three detectors
    detIDList = range(1,4)
    detPosList = [ V3D(9.0,0.0,0.0), V3D(10.0,3.0,0.0), V3D(12.0,3.0,6.0)]
    for j in range(len(detIDList)):
        nextRow = { 'Detector ID': detIDList[j], 
                    'Detector Name': "Detector {0}".format(detIDList[j]),
                    'Detector Position': detPosList[j] }
        tableWS.addRow ( nextRow )

.. testcode:: TableWorspaceProperties

    #setup as above

    # Rows
    print("Row count: {}".format(tableWS.rowCount()))
    print("Detector Position: {0}, Detector Name: {1}, Detector ID: {2}".format(
                    tableWS.row(0)["Detector Position"],
                    tableWS.row(0)["Detector Name"],
                    tableWS.row(0)["Detector ID"])) # row values as a dictionary
    # Resize the table
    tableWS.setRowCount(4)
    # Add Rows
    tableWS.addRow( [2, "new Detector 1", V3D(2,2,2)])
    # or using a dictionary
    nextRow = { 'Detector ID': 5, 
                    'Detector Name': "new Detector 2",
                    'Detector Position':  V3D(5,5,5) }
    tableWS.addRow ( nextRow )

    # Columns
    print("Column count: {}".format(tableWS.columnCount()))
    print("Column names: {}".format(tableWS.getColumnNames()))
    columnValuesList = tableWS.column(0)

    # convert table to dictionary
    data = tableWS.toDict()
    print("Detector names: {}".format(data['Detector Name']))

    # To remove a column
    tableWS.removeColumn("Detector Name")

.. testoutput:: TableWorspaceProperties
    :hide:
    :options: +NORMALIZE_WHITESPACE

    Row count: 3
    Detector Position: [9,0,0], Detector Name: Detector 1, Detector ID: 1
    Column count: 3
    Column names: ['Detector ID', 'Detector Name', 'Detector Position']
    Detector names: ['Detector 1', 'Detector 2', 'Detector 3', '', 'new Detector 1', 'new Detector 2']


Converting To Pandas DataFrames
###############################

Table workspaces can be easily converted to a pandas :class:`~pandas.DataFrame` using the following code snippet.

.. code-block:: python

    import pandas as pd
    df = pd.DataFrame(table.toDict())

If only a subset of the data from the table is required, or you're working with an existing :class:`~pandas.DataFrame` and want to append columns from the Table workspace this can be achieved as follows.

.. code-block:: python

    df = pd.DataFrame()
    for col in tableWS.getColumnNames():
        df[col] = tableWS.column(col)

Pickling Workspaces
###################

A TableWorkspace may be `pickled <https://docs.python.org/2/library/pickle.html/>` and de-pickled in python. Users should prefer using cPickle over pickle, and make sure that the protocol option is set to the HIGHEST_PROTOCOL to ensure that the serialization/deserialization process is as fast as possible.

.. code-block:: python   

  import cPickle as pickle
  pickled = pickle.dumps(ws2d, pickle.HIGHEST_PROTOCOL)

Working with Table Workspaces in C++
------------------------------------

Table workspaces can be created using the workspace factory:

``ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable("TableWorkspace");``

Columns are added using the addColumn method:

| ``table->addColumn("str","Parameter Name");``
| ``table->addColumn("double","Value");``
| ``table->addColumn("double","Error");``
| ``table->addColumn("int","Index");``

Here the first argument is a symbolic name of the column's data type and
the second argument is the name of the column. The predefined types are:

+-----------------+-------------------------+
| Symbolic name   | C++ type                |
+=================+=========================+
| int             | int                     |
+-----------------+-------------------------+
| float           | float                   |
+-----------------+-------------------------+
| double          | double                  |
+-----------------+-------------------------+
| bool            | bool                    |
+-----------------+-------------------------+
| str             | std::string             |
+-----------------+-------------------------+
| V3D             | Mantid::Geometry::V3D   |
+-----------------+-------------------------+
| long64          | int64\_t                |
+-----------------+-------------------------+
| vector_int      | std::vector<int>        |
+-----------------+-------------------------+
| vector_double   | std::vector<double>     |
+-----------------+-------------------------+

The data in the table can be accessed in a number of ways. The most
simple way is to call templated method T& cell(row,col), where col is
the index of the column in the workspace and row is the index of the
cell in the column. Columns are indexed in the order they are created
with addColumn. There are also specialized methods for four predefined
data types: int& Int(row,col), double& Double(row,col), std::string&
String(row,col), bool& Bool(row,col). Columns use std::vector to store
the data. To get access to the vector use getVector(name). To get the
column object use getColumn(name).

Only columns of type int, double and str can currently be saved to Nexus
by :ref:`SaveNexus <algm-SaveNexus>` or
:ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`. Columns of other types will
simply be omitted from the Nexus file without any error message.

Table rows
##########

Cells with the same index form a row. TableRow class represents a row.
Use getRow(int) or getFirstRow() to access existing rows. For example:

.. code-block:: c
 
    std::string key;
    double value;
    TableRow row = table->getFirstRow();
    do
    {
      row >> key >> value;
      std::cout << "key=" << key << " value=" << value << std::endl;
    }
    while(row.next());

TableRow can also be use for writing into a table:

.. code-block:: c

    for(int i=0; i < n; ++i)
    {
        TableRow row = table->appendRow();
        row << keys[i] << values[i];
    }

Defining new column types
#########################

Users can define new data types to be used in TableWorkspace.
TableColumn.h defines macro
DECLARE\_TABLECOLUMN(c\_plus\_plus\_type,symbolic\_name).
c\_plus\_plus\_type must be a copyable type and operators << and >> must
be defined. There is also DECLARE\_TABLEPOINTERCOLUMN macro for
declaring non-copyable types, but it has never been used.




.. include:: WorkspaceNavigation.txt



.. categories:: Concepts
