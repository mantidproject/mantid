.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Extracts a single spectrum from a :ref:`Workspace2D <Workspace2D>` and
stores it in a new workspace.

Usage
-----

**Example**

.. testcode:: ExExtractSingleSpectrum

    ws = CreateSampleWorkspace()
    print "Workspace %s contains %i spectra" % (ws, ws.getNumberHistograms())
    print "Counts for every 10th bin for workspace index 5"
    print ws.readY(5)[0:100:10]

    wsOut = ExtractSingleSpectrum(ws,WorkspaceIndex=5)
    print "After extracting one spectra at workspace index 5"

    print "Workspace %s contains %i spectra" % (wsOut, wsOut.getNumberHistograms())
    print "Counts for every 10th bin for workspace index 0 (now it's 0 as wsOut only contains 1 spectra)"
    print wsOut.readY(0)[0:100:10]


Output:

.. testoutput:: ExExtractSingleSpectrum

    Workspace ws contains 200 spectra
    Counts for every 10th bin for workspace index 5
    [  0.3   0.3   0.3   0.3   0.3  10.3   0.3   0.3   0.3   0.3]
    After extracting one spectra at workspace index 5
    Workspace wsOut contains 1 spectra
    Counts for every 10th bin for workspace index 0 (now it's 0 as wsOut only contains 1 spectra)
    [  0.3   0.3   0.3   0.3   0.3  10.3   0.3   0.3   0.3   0.3]

.. categories::
