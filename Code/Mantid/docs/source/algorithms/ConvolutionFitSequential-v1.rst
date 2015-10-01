
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

An algorithm designed mainly as a sequential call to PlotPeakByLogValue 
but used within the ConvFit tab within the Indirect Analysis interface 
to fit Convolution Functions.

Workflow
--------

.. diagram:: ConvolutionFitSequential-v1_wkflw.dot


Usage
-----

**Example - ConvolutionFitSequential**

.. testcode:: ConvolutionFitSequentialExample

  # Create a host workspace
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

  # Set up algorithm parameters
  function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,Workspace=__ConvFit_Resolution,WorkspaceIndex=0;((composite=ProductFunction,NumDeriv=false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)))"
  bgType = "Fixed Flat"
  startX = -0.547608
  endX = 0.543217
  specMin = 0
  specMax = sample.getNumberHistograms() - 1
  convolve = True
  minimizer = "Levenberg-Marquardt"
  maxIt = 500
  
  # Build resolution workspace (normally done by the Convfit tab when files load)
  AppendSpectra(InputWorkspace1=resolution.getName(), InputWorkspace2=resolution.getName(), OutputWorkspace="__ConvFit_Resolution")
  for i in range(1, sample.getNumberHistograms()):
    AppendSpectra(InputWorkspace1="__ConvFit_Resolution", InputWorkspace2=resolution.getName(), OutputWorkspace="__ConvFit_Resolution")  
  
  # Run algorithm
  wsName = ConvolutionFitSequential(InputWorkspace=sample, Function=function ,BackgroundType=bgType, StartX=startX, EndX=endX, SpecMin=specMin, SpecMax=specMax, Convolve=convolve, Minimizer=minimizer, MaxIterations=maxIt)

  # Obtain result
  result_ws = mtd[wsName]
  
  print "Result has %i Spectra" %result_ws.getNumberHistograms()
  
  print "Amplitude 0: %.5f" %(result_ws.readY(0)[0])
  print "Amplitude 1: %.5f" %(result_ws.readY(0)[1])
  print "Amplitude 2: %.5f" %(result_ws.readY(0)[2])
  
  print "X axis at 0: %.5f" %(result_ws.readX(0)[0])
  print "X axis at 1: %.5f" %(result_ws.readX(0)[1])
  print "X axis at 2: %.5f" %(result_ws.readX(0)[2])
  
  print "Amplitude Err 0: %.5f" %(result_ws.readE(0)[0])
  print "Amplitude Err 1: %.5f" %(result_ws.readE(0)[1])
  print "Amplitude Err 2: %.5f" %(result_ws.readE(0)[2])

Output:  
  
.. testoutput:: ConvolutionFitSequentialExample
  :options: +NORMALIZE_WHITESPACE
  
  Result has 2 Spectra
  
  Amplitude 0: 4.29258
  Amplitude 1: 4.17928
  Amplitude 2: 3.97924

  X axis at 0: 0.52531
  X axis at 1: 0.72917
  X axis at 2: 0.92340

  Amplitude Err 0: 0.00465
  Amplitude Err 1: 0.00464
  Amplitude Err 2: 0.00504

.. categories::

.. sourcelink::

