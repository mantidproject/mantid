.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a simultaneous fit involving a convolution with a defined resolution. This algorithm is a special-case of
:ref:`QENSFitSimultaneous <algm-QENSFitSimultaneous>`, which calculates the elastic incoherent scattering factor when
a delta function is provided in the fitting model.

Workflow
--------

.. diagram:: QENSFitSimultaneous-v1_wkflw.dot


Usage
-----

**Example - ConvolutionFitSimultaneous**

.. testcode:: ConvolutionFitSimultaneousExample

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

  # Set up algorithm parameters
  background = LinearBackground(A0=0, A1=0)
  peak_function = Lorentzian(Amplitude=1, PeakCentre=0, FWHM=0.0175)
  resolution_function = Resolution(Workspace=resolution.getName(), WorkspaceIndex=0)
  model = CompositeFunction(background, Convolution(peak_function, resolution_function))
  multi_function = MultiDomainFunction(model, model)
  startX = -0.547608
  endX = 0.543217
  specMin = 0
  specMax = sample.getNumberHistograms() - 1
  convolve = True  # Convolve the fitted model components with the resolution
  minimizer = "Levenberg-Marquardt"
  maxIt = 500

  # Run algorithm (fit spectra 1 and 2)
  result, params, fit_group, status, chi2 = ConvolutionFitSimultaneous(Function=multi_function,
                                                                       InputWorkspace=sample,
                                                                       WorkspaceIndex=0,
                                                                       InputWorkspace_1=sample,
                                                                       WorkspaceIndex_1=1,
                                                                       StartX=startX, EndX=endX,
                                                                       StartX_1=startX, EndX_1=endX,
                                                                       ConvolveMembers=convolve,
                                                                       Minimizer=minimizer,
                                                                       MaxIterations=maxIt,
                                                                       OutputWorkspace="ConvFitResult")

.. categories::

.. sourcelink::
        :filename: ConvolutionFit
