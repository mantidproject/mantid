
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
An algorithm used for fitting QENS-data simultaneously and formatting the output. Uses the
:ref:`Fit <algm-Fit>` algorithm to perform the sequential fit.

The method of providing multiple data-sets to this algorithm is specified in the :ref:`Fit <algm-Fit>`
documentation.

Workflow
--------

.. diagram:: QENSFitSimultaneous-v1_wkflw.dot


Usage
-----

**Example - QENSFitSimultaneous**

.. testcode:: QENSFitSimultaneousExample

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_red.nxs')

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
  result, params, fit_group, status, chi2 = QENSFitSimultaneous(Function=multi_function,
                                                                InputWorkspace=sample,
                                                                WorkspaceIndex=0,
                                                                InputWorkspace_1=sample,
                                                                WorkspaceIndex_1=1,
                                                                StartX=startX, EndX=endX,
                                                                StartX_1=startX, EndX_1=endX,
                                                                ConvolveMembers=convolve,
                                                                Minimizer=minimizer,
                                                                MaxIterations=maxIt,
                                                                OutputWorkspace="QENSFitResult")

.. categories::

.. sourcelink::
        :filename: QENSFitSimultaneous
