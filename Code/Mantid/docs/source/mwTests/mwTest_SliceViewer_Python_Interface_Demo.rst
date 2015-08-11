:orphan:

.. testsetup:: mwTest_SliceViewer_Python_Interface_Demo[3]

   from mantidplot import *

.. testcode:: mwTest_SliceViewer_Python_Interface_Demo[3]

   # Creating a multidimensional workspace - this will take a while and a lot of memory
   SXD23767 = Load(Filename='SXD23767.raw', LoadMonitors='Exclude')
   mdws=ConvertToMD( InputWorkspace=SXD23767, QDimensions="Q3D",
                       dEAnalysisMode="Elastic", QConversionScales="Q in A^-1",
      	            LorentzCorrection='1', MinValues=[-15,-15,-15], MaxValues=[15,15,15],
                       SplitInto='2', SplitThreshold='50',MaxRecursionDepth='14' )
   
   # Plot a slice with all defaults applied
   svw = plotSlice(mdws, xydim=['Q_lab_x','Q_lab_y'], slicepoint=[0,0,5.8], colorscalelog=True) 
   
   # Specifiying the slice location
   # Move the slice point 
   svw.setSlicePoint('Q_lab_z', 5.9)
   
   #Plotting a line integration
   lv = svw.showLine(start=[-7, 6], end=[7, 6], width=0.1)
   
   # Overplotting a peaks workspace
   peaksws = Load('peaks_qLab.nxs')
   sv = svw.getSlicer()
   sv.setPeaksWorkspaces(['peaksws'])

.. testoutput:: mwTest_SliceViewer_Python_Interface_Demo[3]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Log color scale not possible.


