.. _Table Workspaces:

Table Workspaces
================

-  This page focusses on dealing with Table Workspaces in C++, and is
   aimed at developers. For details on interacting with Table Workspaces
   in Python, please see :py:obj:`this page <mantid.api.ITableWorkspace>`.

Overview
--------

Table workspaces are general purpose workspaces for storing data of
mixed types. A table workspace is organized in columns. Each column has
a name and a type - the type of the data in that column. Table wokspaces
can be created using the workspace factory:

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

The data in the table can be accessed in a number of ways. The most
simple way is to call templated method T& cell(row,col), where col is
the index of the column in the workspace and row is the index of the
cell in the comlumn. Colunms are indexed in the order they are created
with addColumn. There are also specialized methods for four predefined
data types: int& Int(row,col), double& Double(row,col), std::string&
String(row,col), bool& Bool(row,col). Columns use std::vector to store
the data. To get access to the vector use getVector(name). To get the
column object use getColumn(name).

Only columns of type int, double and str can currently be saved to Nexus
by :ref:`SaveNexus <algm-SaveNexus>` or
:ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`. Columns of other types will
simply be ommitted from the Nexus file without any error message.

Table rows
----------

Cells with the same index form a row. TableRow class represents a row.
Use getRow(int) or getFirstRow() to access existing rows. For example:

| ``std::string key;``
| ``double value;``
| ``TableRow row = table->getFirstRow();``
| ``do``
| ``{``
| ``  row >> key >> value;``
| ``  std::cout << "key=" << key << " value=" << value << std::endl;``
| ``}``
| ``while(row.next());``

TableRow can also be use for writing into a table:

| ``for(int i=0; i < n; ++i)``
| ``{``
| ``  TableRow row = table->appendRow();``
| ``  row << keys[i] << values[i];``
| ``}``

Defining new column types
-------------------------

Users can define new data types to be used in TableWorkspace.
TableColumn.h defines macro
DECLARE\_TABLECOLUMN(c\_plus\_plus\_type,symbolic\_name).
c\_plus\_plus\_type must be a copyable type and operators << and >> must
be defined. There is also DECLARE\_TABLEPOINTERCOLUMN macro for
declaring non-copyable types, but it has never been used.



.. categories:: Concepts