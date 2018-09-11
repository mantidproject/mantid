.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

If the specified rows exist they will be deleted form the workspace. If
the row list is empty the algorithm does nothing.

Usage
-----

**Example**

.. testcode:: ExDeteleTableRows

    t=WorkspaceFactory.createTable()
    t.addColumn("double","A")
    t.addColumn("double","B")
    t.addColumn("double","BError")
    for i in range(20):
        t.addRow([i,i*2,1])

    #add it to the Mantid workspace list
    mtd.addOrReplace("myTable",t)

    #delete a single row
    DeleteTableRows(t,Rows=0) #The row index starts at 0

    #delete a List of rows, you can also refer to the workspace using the name in the Workspace List
    DeleteTableRows(t,Rows=[2,4,6,8]) #Note: the previous delete will have moved all the rows up 1

    #delete a range of rows, you can also refer to the workspace using the name in the Workspace List
    DeleteTableRows("myTable",Rows=range(7,14)) 

    print("The remaining values in the first column")
    print(t.column(0))

Output:

.. testoutput:: ExDeteleTableRows

    The remaining values in the first column
    [1.0, 2.0, 4.0, 6.0, 8.0, 10.0, 11.0, 19.0]

.. categories::

.. sourcelink::
