.. _03_pim_sol:

============================
Python in Mantid: Exercise 3
============================

Using ISIS Data
===============

Graphing
--------

.. code-block:: python

    # -----------------------------------------------------
    #  Python Training Exercise 3 Solution.
    #  Controlling the MantidPlot items
    #------------------------------------------------------
    # Perform some algorithms to so that we have some sensible data to plot and look at
    ws=Load(Filename="GEM40979.raw", SpectrumMin=431, SpectrumMax=750)

    # Convert to dSpacing
    ws=ConvertUnits(InputWorkspace=ws, Target= "dSpacing")

    # Smooth the data
    ws=SmoothData(InputWorkspace=ws, NPoints=20)

    # Plot three spectra
    g1 = plotSpectrum(ws, [0,1,2])

    # Set the scales on the x- and y-axes
    layer = g1.activeLayer()
    layer.setAxisScale(Layer.Bottom, 4, 6)
    layer.setAxisScale(Layer.Left, 0, 5e3)

    # Optionally rename the curve titles
    layer.setCurveTitle(0, "bank2, spectrum " + str(431))
    layer.setCurveTitle(1, "bank2, spectrum " + str(432))
    layer.setCurveTitle(2, "bank2 spectrum "+ str(433))

    # Plot index 5
    g2 = plotSpectrum(ws,[5])

    # Merge the plots
    g3=mergePlots(g1, g2)

    mergedLayer= g3.activeLayer()
    mergedLayer.setAxisTitle(Layer.Bottom, "x-axis")
    mergedLayer.setAxisTitle(Layer.Left, "y-axis")

Instrument view
---------------

.. code-block:: python

    import os

    run = Load('LOQ48097')
    instrument_view = getInstrumentView(run.name())
    render = instrument_view.getTab(InstrumentWidget.RENDER)
    render.changeColorMap(os.path.join(config['colormaps.directory'], "Beach.map"))
    render.setMinValue(0)
    render.setMaxValue(195) 
    instrument_view.show()

Using SNS Data
==============

Graphing
--------

.. code-block:: python

    run = Load('Training_Exercise3a_SNS.nxs')
    graph1 = plotSpectrum(source=run, indices=0)
    graph2 = plotSpectrum(source=run, indices=1)
    mergePlots(graph1, graph2)
    layer = graph1.activeLayer()
    layer.setAxisScale(Layer.Bottom, -1.5, 1.8)
    layer.logYlinX()

Instrument view
---------------

.. code-block:: python

    import os 

    run = Load('Training_Exercise3b_SNS.nxs')
    instrument_view = getInstrumentView(run.name())
    render = instrument_view.getTab(InstrumentWidget.RENDER)
    render.changeColorMap(os.path.join(config['colormaps.directory'], "BlackBodyRadiation.MAP"))
    render.setMinValue(0)
    render.setMaxValue(2000) 
    instrument_view.show()