.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compute the cross correlation function for a range of spectra with respect to a reference spectrum.

This is use in powder diffraction experiments when trying to estimate the offset (in number of bin indices) of one spectra with respect to another one.
The input spectra should be in d-spacing and then interpolate on the X-axis of the reference before calculating the cross correlation in bin index.
The cross correlation function is computed

.. math:: c_{fg}(r) = \frac{1}{\sigma_f \sigma_g} \int_{XMin}^{XMax} (f(x) - \mu_f) (g(x+r)-\mu_g) dx

where :math:`x` and :math:`r` are bin indices.
:math:`r` being either half of the number of points in the requested range, ``XMin`` to ``XMax``, of the reference spectrum, or up to `MaxDSpaceShift`, whichever is smaller.
:math:`f(x)` is the reference spectrum, with :math:`\mu_f` and :math:`\sigma_f` being the mean and variance respectively.
Similarly, :math:`g(x)` is the reference spectrum, with :math:`\mu_g` and :math:`\sigma_g` being the mean and variance respectively.
All functions are evaluated as a function of bin number rather than x-value.

More details can be found
`here. <http://en.wikipedia.org/wiki/Cross-correlation>`__

Comments for creating useful cross-correlation output
#####################################################

* Supply input data with relatively high signal to noise ratio.
  When this is not met, the resulting cross-correlation generally have issues having offsets determined from them by :ref:`algm-GetDetectorOffsets`.
* Take note of input binning structure before using :ref:`algm-GetDetectorOffsets` and :ref:`algm-ConvertDiffCal`.
  The different binning modes (linear or logorithmic) correspond to different behavior in the follow-on algorithms.
* Cross-correlating linear binning generally uses ``OffsetMode="Relative"`` or ``OffsetMode="Absolute"`` and requires a narrow range of integration around a single Bragg peak with known reference d-spacing.
* Use a large range for ``XMin`` to ``XMax``.
  This will promote calibrations that take a larger range of the data into account rather than a single diffraction peak.
  However, it only works for logorithmic binning which will use ``OffsetMode="Signed"`` in :ref:`algm-GetDetectorOffsets`.
* With wide range of integration, use the ``MaxDSpaceShift`` to limit the value of :math:`r`.
  Not only will this speed up the calculation, but the output spectra will only include where the spectra are similar to the reference and not have spurious peaks or background.

Usage
-----
**Example - Crosscorrelate 2 spectra**

.. testcode:: ExCrossCorrelate


   #Create a workspace with 2 spectra with five bins of width 0.5
   ws = CreateSampleWorkspace(BankPixelWidth=1, XUnit='dSpacing', XMax=5, BinWidth=0.5)
   ws = ScaleX(InputWorkspace='ws', Factor=0.5, Operation='Add', IndexMin=1, IndexMax=1)
   # Run algorithm  CrossCorrelate
   OutputWorkspace = CrossCorrelate(InputWorkspace='ws', WorkspaceIndexMax=1, XMin=2, XMax=4)

   # Show workspaces
   print("AutoCorrelation {}".format(OutputWorkspace.readY(0)))
   print("CrossCorrelation {}".format(OutputWorkspace.readY(1)))

.. testoutput:: ExCrossCorrelate

   AutoCorrelation [-0.01890212  1.         -0.01890212]
   CrossCorrelation [-0.68136257  0.16838401  0.45685055]

.. categories::

.. sourcelink::
