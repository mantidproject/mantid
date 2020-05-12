.. _03_matrix_ws_py:

===========================
MatrixWorkspace with Python
===========================


MatrixWorkspaces are the most frequently used workspace type in Mantid. The two most commonly used implementations are Workspace2D and EventWorkspace. A Workspace2D stores the data in a histogram form (most commonly) or as point data for each spectra, an EventWorkspace stores a list of raw observations against each spectra. More detail can be found by looking at the individual workspace pages for these workspace types.

#MatrixWorkspaceHierachy.png


Working with MatrixWorkspaces
=============================

Loading a Workspace2D and extracting some basic details:

.. code-block:: python

    ws2D = Load(Filename="LOQ49886.nxs")

    # Basic queries 
    print("Number of histograms: " + str(ws2D.getNumberHistograms()))
    print("Is histogram data: " + str(ws2D.isHistogramData()))
    print("Number of bins: " + str(ws2D.blocksize()))

    # More advanced queries 
    spectrumAxis = ws2D.getAxis(1)
    print("Is spectra axis: " + str(spectrumAxis.isSpectra()))
    print("Number of spectra: " + str(spectrumAxis.length()))

    xAxis = ws2D.getAxis(0)
    xUnit = xAxis.getUnit()
    print("X-Unit: "  + str(xUnit.unitID() + ', ' + xUnit.caption() + ', ' + str(xUnit.symbol())))

    # Get x-axis data as a NumPy array
    xData = ws2D.readX(9683)
    print("X-Data type: " + str(type(xData)))
    print("X-Data:")
    print(xData)

    # Get y-axis data and error data
    print("Y-Data:")
    print(ws2D.readY(9683))
    print("E-Data:")
    print(ws2D.readE(9683))

    # Looping over each spectrum and obtaining a read-only reference to the counts
    for i in range(ws2D.getNumberHistograms()):
        counts = ws2D.readY(i)


Working with EventWorkspaces
============================

Loading a Workspace checking it's type, and reading the number of events.

.. code-block:: python

    eventWS = Load(Filename="CNCS_7860_event.nxs")
    print "Type of Workspace: ", eventWS.id()
    print  "EventWorkspace called %s contains %s events" %(eventWS.name(), eventWS.getNumberEvents())


Rebinning EventWorkspaces
=========================

#Binning example.png

.. code-block:: python

    rebinned = Rebin(InputWorkspace=eventWS,Params='1000')
    rebinnedToWorkspace2D = Rebin(InputWorkspace=eventWS,Params='1000', PreserveEvents=False)
