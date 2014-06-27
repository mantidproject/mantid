.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a single spectrum Workspace2D with X,Y, and E copied from an
first non-integrated dimension of a IMDHistoWorkspace.

Usage
-----

**Example**

.. testcode:: ExHasUB

    ws = CreateMDHistoWorkspace(SignalInput='1,2,3,4,5,6,7,8,9', 
        ErrorInput='1,1,1,1,1,1,1,1,1', Dimensionality='2',
        Extents='-1,1,-1,1', NumberOfBins='3,3', Names='A,B', Units='U,T')

    print "%s is a %s" % (ws, ws.id())

    wsOut=ConvertMDHistoToMatrixWorkspace(ws)

    print "%s is a %s with %i histograms and %i bins" % (wsOut, wsOut.id(), wsOut.getNumberHistograms(), wsOut.blocksize())

Output:

.. testoutput:: ExHasUB

    ws is a MDHistoWorkspace
    wsOut is a Workspace2D with 1 histograms and 3 bins



.. categories::
