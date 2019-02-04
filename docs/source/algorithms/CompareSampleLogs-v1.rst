.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Utility algorithm. Compares specified sample logs for a given list of workspaces or workspace groups. If sample logs match, no output will be produced. If sample logs do not match or do not exist, comma separated list of these sample logs will be returned. This list can be used as an input for :ref:`algm-CreateLogPropertyTable` algorithm to get a TableWorkspace with not identical properties.

For the moment, algorithm does not support comparison of the time series logs. 


Usage
-----

**Example 1: compare identical sample logs**

.. testcode:: ExCompareSampleLogs

    # create workspaces with some sample logs
    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()

    lognames = 'omega,wavelength,polarisation,flipper'
    logvalues = '10.0,4.2,x,ON'
    AddSampleLogMultiple(Workspace=ws1, LogNames=lognames, LogValues=logvalues, ParseType=True)
    logvalues = '10.0,4.2,x,ON'
    AddSampleLogMultiple(Workspace=ws2, LogNames=lognames, LogValues=logvalues, ParseType=True)

    # compare sample logs
    result = CompareSampleLogs('ws1,ws2', 'omega,wavelength,polarisation,flipper' , 0.01)
    if result == '':
        print("All sample logs match!")

.. testcleanup:: ExCompareSampleLogs

    DeleteWorkspace('ws1')
    DeleteWorkspace('ws2')

Output:

.. testoutput:: ExCompareSampleLogs

    All sample logs match!


**Example 2: create a table of not identical sample logs**

.. testcode:: ExCompareSampleLogs2

    # create workspaces with some sample logs
    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()

    lognames = 'run_title,omega,wavelength,polarisation,flipper'
    logvalues = 'ws1,10.0,4.2,x,ON'
    AddSampleLogMultiple(Workspace=ws1, LogNames=lognames, LogValues=logvalues, ParseType=True)
    logvalues = 'ws2,12.0,4.2,x,OFF'
    AddSampleLogMultiple(Workspace=ws2, LogNames=lognames, LogValues=logvalues, ParseType=True)

    # compare sample logs
    result = CompareSampleLogs('ws1,ws2', lognames , 0.01)
    print("Following sample logs do not match:  {}".format(result))

    # create a table
    table = CreateLogPropertyTable('ws1,ws2', result, GroupPolicy='All')
    print("Column names are:  {}".format(table.getColumnNames()))
    print("The omega values are: {}".format(table.column(1)))
    print("The flipper values are: {}".format(table.column(2)))

.. testcleanup:: ExCompareSampleLogs2

    DeleteWorkspace('ws1')
    DeleteWorkspace('ws2')

Output:

.. testoutput:: ExCompareSampleLogs2

    Following sample logs do not match:  run_title,omega,flipper
    Column names are:  ['run_title', 'omega', 'flipper']
    The omega values are: ['10', '12']
    The flipper values are: ['ON', 'OFF']

.. categories::

.. sourcelink::
