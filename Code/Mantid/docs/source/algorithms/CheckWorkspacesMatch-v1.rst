.. algorithm::

.. summary::

.. alias::

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

Usage
-----

**Example - check that two workspaces are equal to one another:**  

.. testcode:: ExCheckWorkspacesMatchSimple

    dataX = [0,1,2,3,4,5,6,7,8,9]
    dataY = [1,1,1,1,1,1,1,1,1]
    ws1 = CreateWorkspace(dataX, dataY)

    #create a copy of the workspace
    ws2 = CloneWorkspace(ws1)

    print CheckWorkspacesMatch(ws1, ws2)


Output:

.. testoutput:: ExCheckWorkspacesMatchSimple
   
    Success!

**Example - check that two workspaces match within a certain tolerance:**  

.. testcode:: ExCheckWorkspacesMatchTolerance

    import numpy as np

    #create a workspace with some simple data
    dataX = range(0,20)
    dataY1 = np.sin(dataX)
    ws1 = CreateWorkspace(dataX, dataY1)

    #create a similar workspace, but with added noise
    dataY2 = np.sin(dataX) + 0.1*np.random.random_sample(len(dataX))
    ws2 = CreateWorkspace(dataX, dataY2)

    print CheckWorkspacesMatch(ws1, ws2) #fails, they're not the same
    print CheckWorkspacesMatch(ws1, ws2, Tolerance=0.1) #passes, they're close enough


Output:

.. testoutput:: ExCheckWorkspacesMatchTolerance
   
    Data mismatch
    Success!


.. categories::

