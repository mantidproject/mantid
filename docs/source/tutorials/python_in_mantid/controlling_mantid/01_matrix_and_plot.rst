.. _01_matrix_and_plot:

=======================
Matrix and Plot Control
=======================


Dragging a workspace from the Mantid Workspaces list in to main area creates a Mantid Matrix, which displays the underlying data in a worksheet.

To do this from Python use the importMatrixWorkspace("workspace-name") function, giving the name of the workspace to import, e.g.

.. code-block:: python

    RawData = Load("MAR11015")
    workspace_mtx = importMatrixWorkspace("RawData")

#Using a Mantid matrix to plot a spectrum

Right clicking on a row or a column allows a plot of that spectrum or time bin, which can also be achieved in Python using the plotSpectrum() or plotBin() functions, e.g.

.. code-block:: python

    graph_spec = plotSpectrum(RawData, 0)
    graph_time = plotBin(RawData, 0)

To include error bars on a plot, there is an extra argument that can be set to 'True' to enable them, e.g.

.. code-block:: python

    graph_spec = plotSpectrum(RawData, 0, error_bars=True)

The handle returned by the plot commands, graph_spec and graph_time above, can be used to adjust the properties of the plot, for example setting scales, axis titles, plot titles etc. For example,

.. code-block:: python

    # Get the active layer of the the graph
    l = graph_spec.activeLayer()

    # Rescale the x-axis to a show a smaller region
    l.setAxisScale(Layer.Bottom, 2.0, 2.5) 

    # Retitle the y-axis
    l.setAxisTitle(Layer.Left, "Cross-section")

    # Give the graph a new title
    l.setTitle("Cross-section vs wavelength")

    # Put y-axis to a log scale 
    l.setAxisScale(Layer.Left, 1, 1000, Layer.Log10)

    # Change the title of a curve
    l.setCurveTitle(0, "My Title")

Multiple workspaces/spectra can be plotted by providing lists within the plotSpectrum() or plotBin() functions, e.g.

.. code-block:: python

    # Plot multiple spectra from a single workspace
    g1 = plotSpectrum(RawData, [0,1,3])

    # Here you need to load files into RawData1 and RawData2 before the next two commands

    # Plot a spectra across multiple workspaces
    g2 = plotSpectrum([RawData1,RawData2], 0)

    # Plot multiple spectra across multiple workspaces
    g2 = plotSpectrum([RawData1,RawData2], [0,1,3])

Existing plots can be merged with the mergePlots() function although the above functions are preferred

.. code-block:: python

    # g1,g2 are already created graphs
    mergePlots(g1, g2) # All of the curves from g2 will be merged on to g1

Contour and 3D plots can be generated from an imported matrix workspace (see above). The commands return a handle that can be used to adjust the properties of the plot in the manner shown above.

.. code-block:: python

    graph_2d = workspace_mtx.plotGraph2D()
    graph_3d = workspace_mtx.plotGraph3D()

More detailed information on controlling plotting for MantidPlot within Python can be found in QtiPlot's documentation here.

