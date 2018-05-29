
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
An algorithm used for fitting QENS-data sequentially and formatting the output. Uses the
:ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>` algorithm to perform the sequential fit.

The string format expected by the "Input" property of this algorithm is outlined in
:ref:`PlotPeakByLogValue <algm-PlotPeakByLogValue>`.

Workflow
--------

.. diagram:: QENSFitSequential-v1_wkflw.dot


Usage
-----

**Example - QENSFitSequential**

.. testcode:: QENSFitSequentialExample

  from __future__ import print_function

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