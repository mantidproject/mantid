
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

  # Load sample and resolution files
  sample = Load('irs26176_graphite002_red.nxs')
  resolution = Load('irs26173_graphite002_res.nxs')

  # Set up algorithm parameters
  function = """name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);
  (composite=Convolution,FixResolution=true,NumDeriv=true;
  name=Resolution,Workspace=resolution,WorkspaceIndex=0;
  name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)"""
  startX = -0.547608
  endX = 0.543217
  convolve = True  # Convolve the fitted model components with the resolution
  minimizer = "Levenberg-Marquardt"
  maxIt = 500

  # Run algorithm
  result, params, fit_group = QENSFitSequential(InputWorkspace=sample,
                                                Function=function,
                                                PassWSIndexToFunction=True,
                                                StartX=startX, EndX=endX,
                                                ConvolveMembers=convolve,
                                                Minimizer=minimizer, MaxIterations=maxIt)

  # The QENSFitSequential algorithm can take an optional OutputFitStatus flag, to output the Chi squared and fit status
  # of each fit
  result, params, fit_group, fit_status, chi2 = QENSFitSequential(InputWorkspace=sample,
                                                Function=function,
                                                PassWSIndexToFunction=True,
                                                StartX=startX, EndX=endX,
                                                ConvolveMembers=convolve,
                                                Minimizer=minimizer, MaxIterations=maxIt,
                                                OutputFitStatus=True)


.. categories::

.. sourcelink::
        :filename: QENSFitSequential
