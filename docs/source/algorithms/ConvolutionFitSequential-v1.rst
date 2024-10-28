
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a sequential fit involving a convolution with a defined resolution. This algorithm is a special-case of
:ref:`QENSFitSequential <algm-QENSFitSequential>`, which calculates the elastic incoherent scattering factor when
a delta function is provided in the fitting model.

Workflow
--------

.. diagram:: QENSFitSequential-v1_wkflw.dot


Usage
-----

**Example - ConvolutionFitSequential**

.. testcode:: ConvolutionFitSequentialExample

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

  # Set up algorithm parameters
  function = """name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);
  (composite=Convolution,FixResolution=true,NumDeriv=true;
  name=Resolution,Workspace=resolution,WorkspaceIndex=0;
  name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)"""
  startX = -0.547608
  endX = 0.543217
  specMin = 0
  specMax = sample.getNumberHistograms() - 1
  convolve = True  # Convolve the fitted model components with the resolution
  minimizer = "Levenberg-Marquardt"
  maxIt = 500

  # Run algorithm
  result_ws, _, _ = ConvolutionFitSequential(InputWorkspace=sample,
                                             Function=function,
                                             PassWSIndexToFunction=True,
                                             StartX=startX, EndX=endX,
                                             SpecMin=specMin, SpecMax=specMax,
                                             ConvolveMembers=convolve,
                                             Minimizer=minimizer, MaxIterations=maxIt)
  result_ws = result_ws[0]

  print("Result has %i Spectra" %result_ws.getNumberHistograms())

  print("Amplitude 0: %.3f" %(result_ws.readY(0)[0]))
  print("Amplitude 1: %.3f" %(result_ws.readY(0)[1]))
  print("Amplitude 2: %.3f" %(result_ws.readY(0)[2]))

  print("X axis at 0: %.5f" %(result_ws.readX(0)[0]))
  print("X axis at 1: %.5f" %(result_ws.readX(0)[1]))
  print("X axis at 2: %.5f" %(result_ws.readX(0)[2]))

  print("Amplitude Err 0: %.5f" %(result_ws.readE(0)[0]))
  print("Amplitude Err 1: %.5f" %(result_ws.readE(0)[1]))
  print("Amplitude Err 2: %.5f" %(result_ws.readE(0)[2]))

Output:

.. testoutput:: ConvolutionFitSequentialExample
  :options: +NORMALIZE_WHITESPACE

  Result has 3 Spectra

  Amplitude 0: 4.314
  Amplitude 1: 4.213
  Amplitude 2: 4.555

  X axis at 0: 0.52531
  X axis at 1: 0.72917
  X axis at 2: 0.92340

  Amplitude Err 0: 0.00460
  Amplitude Err 1: 0.00468
  Amplitude Err 2: 0.00577

.. categories::

.. sourcelink::
        :filename: ConvolutionFit
