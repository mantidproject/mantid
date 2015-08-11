:orphan:

.. testsetup:: mwTest_Matrix_and_Plot_Control[4]

   from mantidplot import *

.. testcode:: mwTest_Matrix_and_Plot_Control[4]

   RawData = Load("MAR11015")
   workspace_mtx = importMatrixWorkspace("RawData")


.. testsetup:: mwTest_Matrix_and_Plot_Control[15]

   from mantidplot import *
   RawData = Load("MAR11015")

.. testcode:: mwTest_Matrix_and_Plot_Control[15]

   graph_spec = plotSpectrum(RawData, 0)
   graph_time = plotBin(RawData, 0)


.. testsetup:: mwTest_Matrix_and_Plot_Control[26]

   from mantidplot import *
   RawData = Load("MAR11015")

.. testcode:: mwTest_Matrix_and_Plot_Control[26]

   graph_spec = plotSpectrum(RawData, 0, True)


.. testsetup:: mwTest_Matrix_and_Plot_Control[36]

   from mantidplot import *
   RawData = Load("MAR11015")
   graph_spec = plotSpectrum(RawData, 0, True)

.. testcode:: mwTest_Matrix_and_Plot_Control[36]

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


.. testsetup:: mwTest_Matrix_and_Plot_Control[63]

   from mantidplot import *
   RawData = Load("MAR11015")
   RawData1 = CloneWorkspace(RawData)
   RawData2 = CloneWorkspace(RawData)

.. testcode:: mwTest_Matrix_and_Plot_Control[63]

   # Plot multiple spectra from a single workspace
   g1 = plotSpectrum(RawData, [0,1,3])
   
   # Here you need to load files into RawData1 and RawData2 before the next two commands
   
   # Plot a spectra across multiple workspaces
   g2 = plotSpectrum([RawData1,RawData2], 0)
   
   # Plot multiple spectra across multiple workspaces
   g2 = plotSpectrum([RawData1,RawData2], [0,1,3])


.. testsetup:: mwTest_Matrix_and_Plot_Control[84]

   from mantidplot import *
   RawData = Load("MAR11015")
   RawData1 = CloneWorkspace(RawData)
   RawData2 = CloneWorkspace(RawData)
   g1 = plotSpectrum(RawData, [0,1,3])
   g2 = plotSpectrum([RawData1,RawData2], [0,1,3])

.. testcode:: mwTest_Matrix_and_Plot_Control[84]

   # g1,g2 are already created graphs
   mergePlots(g1, g2) # All of the curves from g2 will be merged on to g1


.. testsetup:: mwTest_Matrix_and_Plot_Control[99]

   from mantidplot import *
   RawData = Load("MAR11015")
   workspace_mtx = importMatrixWorkspace("RawData")

.. testcode:: mwTest_Matrix_and_Plot_Control[99]

   graph_2d = workspace_mtx.plotGraph2D()
   graph_3d = workspace_mtx.plotGraph3D()


.. testsetup:: mwTest_Matrix_and_Plot_Control[119]

   from mantidplot import *
   RawData = Load("MAR11015")
   RawData1 = CloneWorkspace(RawData)
   RawData2 = CloneWorkspace(RawData)

.. testcode:: mwTest_Matrix_and_Plot_Control[119]

   # Plotting spectra using the functional interface
   plot([RawData1,RawData2],  [0,1,3], tool='plot_spectrum', linestyle='-.')
   title("Cross-section vs wavelength")
   ylabel("Cross-section")
   xlim(2, 2.5)
   yscale('log')
   ylim(1,1000)
   #
   # Now plotting the same using the object-oriented interface
   lines = plot([RawData1,RawData2],  [0,1,3], tool='plot_spectrum', linestyle='-.')
   fig = lines[0].figure()
   fig.suptitle("Cross-section vs wavelength")
   ax = fig.axes()[0]
   ax.set_ylabel("Cross-section")
   ax.set_xlim(2, 2.5)
   ax.set_yscale('log')
   ax.set_ylim(1,1000)


