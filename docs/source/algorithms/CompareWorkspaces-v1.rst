
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compares two workspaces for equality. This algorithm is mainly intended
for use by Mantid developers as part of the testing process.

The data values (X,Y and error) are always checked. The algorithm can
also optionally check the axes (this includes the units), the
spectra-detector map, the instrument (the name and parameter map) and
any bin masking.

In the case of :ref:`EventWorkspaces <EventWorkspace>`, they are checked to
hold identical event lists. Comparisons between an EventList and a
Workspace2D always fail.

This algorithm has two outputs: A boolean "Result" that indicates whether
the workspaces matched (true) or not (false), and a TableWorkspace property
named "Messages" (workspace name defaults to "compare_msgs") with three
columns. The first column contains messages about any mismatches that were
detected. The second and third columns are the names of the workspaces that
were being compared when the mismatch message was issues. If all the input
workspaces matched, the message workspace will be empty.

Please note that details about the comparisons will be only available when the log level is set to debug.

Checking option `CheckInstrument` will enable the following comparisons between the instruments embedded
in each of the two workspaces:

- instrument name
- positions and rotations of detectors
- mask of detectors
- position of the source and sample
- instrument parameters

Usage
-----

**Example - Check that two workspaces are equal to one another:**

.. testcode:: ExCompareWorkspacesSimple

    dataX = [0,1,2,3,4,5,6,7,8,9]
    dataY = [1,1,1,1,1,1,1,1,1]
    ws1 = CreateWorkspace(dataX, dataY)

    #create a copy of the workspace
    ws2 = CloneWorkspace(ws1)

    (result, messages) = CompareWorkspaces(ws1, ws2)

    print("Result: {}".format(result))
    print(messages.rowCount())


Output:

.. testoutput:: ExCompareWorkspacesSimple

    Result: True
    0


**Example - Check that two workspaces match within a certain tolerance:**

.. testcode:: ExCompareWorkspacesTolerance

    import numpy as np

    # Create a workspace with some simple data
    dataX = range(0,20)
    dataY1 = np.sin(dataX)
    ws1 = CreateWorkspace(dataX, dataY1)

    # Create a similar workspace, but with added noise
    dataY2 = np.sin(dataX) + 0.1*np.random.random_sample(len(dataX))
    ws2 = CreateWorkspace(dataX, dataY2)

    (result, messages) = CompareWorkspaces(ws1, ws2) # Fails, they're not the same
    print("Result: {}".format(result))
    print("Displaying {} messages:".format(messages.rowCount()))
    for row in messages:
        print("'Message': '{Message}', 'Workspace 1': '{Workspace 1}', 'Workspace 2': '{Workspace 2}'".format(**row))

    (result, messages) = CompareWorkspaces(ws1, ws2, Tolerance=0.1) # Passes, they're close enough
    print("Result: {}".format(result))
    print("Displaying {} messages:".format(messages.rowCount()))
    for row in messages:
        print("'Message': '{Message}', 'Workspace 1': '{Workspace 1}', 'Workspace 2': '{Workspace 2}'".format(**row))


Output:

.. testoutput:: ExCompareWorkspacesTolerance

    Result: False
    Displaying 1 messages:
    'Message': 'Data mismatch', 'Workspace 1': 'ws1', 'Workspace 2': 'ws2'
    Result: True
    Displaying 0 messages:


.. categories::

.. sourcelink::
