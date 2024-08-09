.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates a single spectrum Workspace2D with X,Y, and E copied from
columns ColumnX, ColumnY, and ColumnE respectively. If ColumnE is not
set the E vector will be filled with 1s. The type of the columns must be
convertible to C++ double.

Usage
-----

**Example**

.. testcode:: ExConvertTabletoMatrix

    t=WorkspaceFactory.createTable()
    t.addColumn("double","A")
    t.addColumn("double","B")
    t.addColumn("double","BError")
    t.addRow([1,2,1])
    t.addRow([3,4,1])
    t.addRow([5,6,1])

    #add it to the Mantid workspace list
    mtd.addOrReplace("myTable",t)

    ws=ConvertTableToMatrixWorkspace(t,"A","B","BError")

    print("{} is a {} and the Y values are:".format(ws,ws.id()))
    print(ws.readY(0))

Output:

.. testoutput:: ExConvertTabletoMatrix

    ws is a Workspace2D and the Y values are:
    [2. 4. 6.]

.. categories::

.. sourcelink::


