.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This utility algorithm is used to control consecutive executions of another algorithm. After defining a table of desired input and output properties, ``WorkflowAlgorithmRunner`` connects the outputs to input properties, declares the order of execution, and runs the algorithm. It is mainly meant for managing the execution of workflow algorithms for data reduction purposes although it can run all types of algorithms.

The runs are specified in a `TableWorkspace` and given as the *SetupTable* input property to the algorihtm. The first column has to contain unique string identifiers for each row. The rest of the columns have to be named same as the input/output properties of the managed algorithm.

Each row in the *SetupTable* corresponds to a single run of the managed algorithm. Its properties are set without modifications from the corresponding columns of the row except for special input/output columns declare in the *InputOutputMap*. For example, two independent runs of the :ref:`algm-Scale` would be specified as

=======  ==============  ===============  ======  =========
Id       InputWorkspace  OutputWorkspace  Factor  Operation 
=======  ==============  ===============  ======  =========
1st run  ws1             ws1              0.42    Multiply  
2nd run  ws2             scaled_ws2       100     Multiply  
=======  ==============  ===============  ======  =========

The *Operation* column could have been omitted in the above to use the default value of the *Operation* property of `Scale`.

*InputOutputMap* connects the outputs of a run to the inputs of another run. It is a `TableWorkspace` where the column names correspond to some or all of the managed algorithm's input properties. The table has only a single row which lists the names of the output properties whose final values will be forwarded to the corresponding input property. To continue the :ref:`algm-Scale` example above, the *InputWorkspace* and *OutputWorkspace* properties would be connected by the following single column table:

+-----------------+
| InputWorkspace  |
+=================+
| OutputWorkspace |
+-----------------+

.. note::
    Only workspace properties can be specified in the *InputOutputMap*.

.. note::
    A single output property can be wired to several input properties. However, an input property cannot have multiple output properties.

If the one wanted to give the workspace scaled by '2nd run' as an input for '1st run', the *InputOutputMap* would look like the following:

=======  ==============  ===============  ======  =========
Id       InputWorkspace  OutputWorkspace  Factor  Operation
=======  ==============  ===============  ======  =========
1st run  2nd run         'ws1'            0.42    Multiply
2nd run  'ws2'           scaled_ws2       100     Multiply
=======  ==============  ===============  ======  =========

This would result in '2nd run' to be executed first to produce the 'scaled_ws2' workspace which in turn would be fed to '1st run' as the *InputWorkspace* property. Behind the scenes, ``WorkflowAlgorithmRunner`` will replace the '2nd run' in the InputWorkspace column of '1st run' by scaled_ws2.

In the above, note the quotation marks around the 'ws2' and 'ws1' property names. These are **hard coded input** and **forced output** values, respectively. This tells `WorkspaceAlgorithmRunner` to omit input/output mapping and to keep these values as they are.

.. note::
    A non-forced output property which is not needed by the other runs will be cleared by ``WorkflowAlgorithmRunner``.

An equivalent python commands to running ``WorkflowAlgorithmRunner`` with the above *InputOutputMap* and *SetupTable* would be

::

    Scale(InputWorkspace='ws2', OutputWorkspace='scaled_ws2', Factor=100, Operation='Multiply')
    Scale(InputWorkspace='scaled_ws2', OutputWorkspace='ws1', Factor=0.42, Operation='Multiply')

Usage
-----

**Example: Let's recreate the example in the Description section**

.. testcode:: ScaleExample

    # This is our initial workspace. We want to scale it first by 100
    # and then by 0.42
    CreateSingleValuedWorkspace(OutputWorkspace='ws2', DataValue=1.0)
    
    # Setup the runs for the Scale algorithm
    setupTable = WorkspaceFactoryImpl.Instance().createTable()
    setupTable.addColumn('str', 'Run name') # First column can have arbitrary name.
    # The rest of the columns can be in arbitrary order
    setupTable.addColumn('str', 'InputWorkspace')
    setupTable.addColumn('double', 'Factor') # Scale expects to get a number here.
    setupTable.addColumn('str', 'OutputWorkspace')
    row = {
        'Run name': '1st run',
        'InputWorkspace': '2nd run',
        'Factor': 0.42,
        'OutputWorkspace': '"ws1"' # Forced output either by '' or "".
    }
    setupTable.addRow(row)
    row = {
        'Run name': '2nd run',
        'InputWorkspace': "'ws2'",
        'Factor': 100,
        'OutputWorkspace': 'scaled_ws2'
    }
    setupTable.addRow(row)
    AnalysisDataServiceImpl.Instance().addOrReplace('setupTable', setupTable)
    
    # Map OutputWorkspace to InputWorkspace
    ioMap = WorkspaceFactoryImpl.Instance().createTable()
    ioMap.addColumn('str', 'InputWorkspace')
    ioMap.addRow({'InputWorkspace': 'OutputWorkspace'})
    AnalysisDataServiceImpl.Instance().addOrReplace('ioMapTable', ioMap)
    
    # Execute the algorithm
    WorkflowAlgorithmRunner('Scale', SetupTable=setupTable, InputOutputMap=ioMap)
    
    # Print some results
    print('Original input value: {0}'.format(mtd['ws2'].dataY(0)[0]))
    print('After scaling by 100: {0}'.format(mtd['scaled_ws2'].dataY(0)[0]))
    print('After further scaling by 0.42: {0}'.format(mtd['ws1'].dataY(0)[0]))

.. testoutput:: ScaleExample

    Original input value: 1.0
    After scaling by 100: 100.0
    After further scaling by 0.42: 42.0

.. categories::

.. sourcelink::
