.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to generate a background for HFIR monochromatic diffraction data. For every
detector pixel it takes a percentile of the intensities recorded along the rotation axis. Bragg
scattering illuminates a pixel for only part of the scan while the background persists, so a
percentile rejects the peaks and retains the goniometer-independent level.

When ``BackgroundWindowSize`` is set, the percentile is taken over a sliding window of that many
rotation steps, allowing the background to vary slowly with rotation angle. It selects a ranked value
in the same way as
`Scipy.ndimage.percentile_filter <https://docs.scipy.org/doc/scipy/reference/generated/scipy.ndimage.percentile_filter.html>`_.
When the property is left unset, the percentile is taken over the whole rotation axis and the
background is constant in rotation for each pixel; the value is interpolated linearly between
order statistics, as
`Numpy.percentile <https://numpy.org/doc/stable/reference/generated/numpy.percentile.html>`_ does.

``BackgroundLevel`` is a percentile and must lie between 0 and 100. It defaults to 50, which
takes the median along the rotation axis.

The optional ``NormalizeBy`` property can be set to ``Time`` or ``Monitor`` to divide each rotation
by its duration or monitor count before calculating the percentile. ``None`` does no normalization.
``NormalizeOutput`` controls the units of the result. When it is ``True``, the output background
remains normalized when the user chooses ``Time`` or ``Monitor``.
When it is ``False`` (the default), the result is multiplied by time duration or monitor count of each rotation.
The corresponding error variances are scaled consistently.

For WAND (HB2C), the ``duration`` and ``monitor_count`` sample logs are used. For DEMAND (HB3A),
the corresponding logs are named ``time`` and ``monitor``. Normalization is not supported for
other instruments.

Uncertainties
-------------

The uncertainty on the output background is treated as the uncertainty of the percentile estimate,
not just as the uncertainty of one selected input value. In other words, when several rotation steps
contribute to the background, the algorithm assumes the combined estimate is more reliable than any
single contributing point.

The amount of this improvement depends on how many rotation steps are used. When
``BackgroundWindowSize`` is unset, all rotations for that detector pixel contribute. When
``BackgroundWindowSize`` is set, only the rotations inside the sliding window contribute. Larger
windows therefore usually give smaller uncertainties, while smaller windows stay closer to the
uncertainty of the selected input value.

The calculation starts from the uncertainty of the input value selected by the percentile. This is
intentional: the percentile is used to avoid Bragg peaks, so the uncertainty estimate also avoids
using those rejected peak values.

There are a few special cases and limitations:

- If the percentile selects the smallest or largest value, the algorithm keeps the uncertainty of
  that selected input value. This includes ``BackgroundLevel`` values of 0 and 100, and can also
  happen for low or high percentiles when a small sliding window is used.
- For a sliding window, the algorithm uses the ranked value that is actually selected from the
  window. With small windows, that rank may only approximate the requested ``BackgroundLevel``.
- If the sliding window is padded by repeating values at the edge of an incomplete rotation, fewer
  independent measurements contribute than the window size suggests, so the reported uncertainty may
  be slightly too small.
- Output uncertainties are correlated with one another. Without a sliding window, the same
  background estimate is reused along the full rotation axis for each pixel. With a sliding window,
  neighbouring rotations share most of the same input values. A ``MDHistoWorkspace`` cannot store
  these correlations, so later operations that combine values along the rotation axis may report
  uncertainties that are too small.

Usage
-----

.. testcode::

   # create workspace
   import numpy as np
   signal = np.random.randint(low=0, high=10, size=(100,100,100))
   workspace = CreateMDHistoWorkspace(SignalInput=signal,
                                      ErrorInput=np.ones_like(signal),
                                      Dimensionality=3,
                                      Extents='0,10,0,10,0,10',
                                      Names='x,y,z',
                                      NumberOfBins='100,100,100',
                                      Units='number,number,number',
                                      OutputWorkspace='output')



   # Perform the background interpolation
   outputWS = HFIRGoniometerIndependentBackground(workspace, BackgroundWindowSize=10)

   # Check output
   print("Shape of the resulting Signal is: {}".format(outputWS.getSignalArray().shape))

Output:

.. testoutput::

   Shape of the resulting Signal is: (100, 100, 100)


.. categories::

.. sourcelink::
