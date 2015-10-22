.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Utility algorithm. Compares specified sample logs for a given list of workspaces or workspace groups. If sample logs match, no output will be produced. If sample logs do not match or do not exist, one two actions on user's choice will be performed:

- **warning**: algorithm will throw a warning, containing run titles or run numbers of not identical properties. All specified sample logs will be checked for all input workspaces. This action may be utilized to warn the user about difference in optional sample logs.

- **error**: algorithm will terminate with an error message after first not matching property will be found or if one of specified sample logs does not exist. This action is useful for mandatory properties, which must be identical in the given list of workspaces.


Usage
-----

.. testcode:: ExCompareSampleLogs

    # create workspaces with some sample logs
    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()

    lognames = 'run_title,deterota,wavelength,polarisation,flipper'
    logvalues = 'ws1,-10.0,4.2,x,ON'
    AddSampleLogMultiple(Workspace=ws1, LogNames=lognames, LogValues=logvalues, ParseType=True)
    logvalues = 'ws2,-10.0,4.2,x,OFF'
    AddSampleLogMultiple(Workspace=ws2, LogNames=lognames, LogValues=logvalues, ParseType=True)

    # compare sample logs
    CompareSampleLogs('ws1,ws2', 'deterota,wavelength,polarisation,flipper' , 0.01, 'warning')

.. testcleanup:: ExCompareSampleLogs

    DeleteWorkspace('ws1')
    DeleteWorkspace('ws2')

Output:

.. testoutput:: ExCompareSampleLogs

    Property flipper does not match! ws1: ON, but ws2: OFF

.. categories::

.. sourcelink::
