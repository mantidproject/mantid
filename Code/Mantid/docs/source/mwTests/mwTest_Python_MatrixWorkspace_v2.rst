:orphan:

.. testcode:: mwTest_Python_MatrixWorkspace_v2[6]

   ws2D = Load(Filename="LOQ49886.nxs")
   
   # Basic queries 
   print "Number of histograms", ws2D.getNumberHistograms()
   print "Is histogram data: ", ws2D.isHistogramData()
   print "Number of bins ", ws2D.blocksize()
   
   # More advanced queries 
   spectrumAxis = ws2D.getAxis(1)
   print  "Is spectra axis: ", spectrumAxis.isSpectra()
   print "Number of spectra: ", spectrumAxis.length()
   
   xAxis = ws2D.getAxis(0)
   xUnit = xAxis.getUnit()
   print "X-Unit: ", xUnit.unitID(), xUnit.caption(), str(xUnit.symbol())
   
   #Looping over each spectrum and obtaining a read-only reference to the counts
   for i in range(ws2D.getNumberHistograms()):
   	counts = ws2D.readY(i)

.. testoutput:: mwTest_Python_MatrixWorkspace_v2[6]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Number of histograms 17792
   Is histogram data:  True
   Number of bins  5
   Is spectra axis:  True
   Number of spectra:  17792
   X-Unit:  TOF Time-of-flight microsecond


.. testcode:: mwTest_Python_MatrixWorkspace_v2[39]

   eventWS = Load(Filename="CNCS_7860_event.nxs")
   print "Type of Workspace: ", eventWS.id()
   print  "EventWorkspace called %s contains %s events" %(eventWS.name(), eventWS.getNumberEvents())

.. testoutput:: mwTest_Python_MatrixWorkspace_v2[39]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Type of Workspace:  EventWorkspace
   EventWorkspace called eventWS contains 112266 events


.. testsetup:: mwTest_Python_MatrixWorkspace_v2[51]

   eventWS = Load(Filename="CNCS_7860_event.nxs")

.. testcode:: mwTest_Python_MatrixWorkspace_v2[51]

   rebinned = Rebin(InputWorkspace=eventWS,Params='1000')
   rebinnedToWorkspace2D = Rebin(InputWorkspace=eventWS,Params='1000', PreserveEvents=False)


