
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

  # Set up algorithm parameters
  function = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);(composite=Convolution,FixResolution=true,NumDeriv=true;name=Resolution,Workspace=__ConvFit_Resolution,WorkspaceIndex=0;(name=DeltaFunction,Height=1;(composite=ProductFunction,NumDeriv=false;name=UserFunction,Formula=((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp))),Temp=1.03,ties=(Temp=1.03);name=Lorentzian,Amplitude=4.20243,PeakCentre=-0.008497,FWHM=0.053436)))"
  bgType = "Fixed Flat"
  startX = -0.547608
  endX = 0.543217
  temp = 1.03
  specMin = 0
  specMax = 50
  convolve = True
  minimizer = "Levenberg-Marquardt"
  maxIt = 500
	
  # Create a host workspace
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_res.nxs')
  
  AppendSpectra(InputWorkspace1=sample.getName(), InputWorkspace2=sample.getName(), OutputWorkspace="__ConvFit_Resolution")
  
  for i in range(1, sample.getNumberHistograms()):
    AppendSpectra(InputWorkspace1="__ConvFit_Resolution", InputWorkspace2=sample.getName(), OutputWorkspace="__ConvFit_Resolution")
  
  
  ConvolutionFitSequential(InputWorkspace=sample, Function=function ,BackgroundType=bgType, StartX=startX, EndX=endX, Temperature=temp, SpecMin=specMin, SpecMax=specMax, Convolve=convolve, Minimizer=minimizer, MaxIterations=maxIt)

  result_ws = mtd["irs26176_graphite002_conv_1LFixF_s0_to_50_Result"]
  

.. categories::

.. sourcelink::

