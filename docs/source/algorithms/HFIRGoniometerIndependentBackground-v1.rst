.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to generate a background for HFIR monochromatic diffraction data. This algorithm wraps
`Scipy.ndimage.percentile_filter <https://docs.scipy.org/doc/scipy/reference/generated/scipy.ndimage.percentile_filter.html>`_
to generate the background for the input workspace. In the case that BackgroundWindowSize is -1,
`Numpy.percentile <https://numpy.org/doc/stable/reference/generated/numpy.percentile.html>`_ is used to generate the background as
it is much faster.

The optional ``NormalizeBy`` property can be set to ``Time`` or ``Monitor`` to divide each rotation
by its duration or monitor count before calculating the percentile. ``None`` does no normalization.
``NormalizeOutput`` controls the units of the result. When it is ``True``, the output background
remains normalized when the user chooses ``Time`` or ``Monitor``.
When it is ``False`` (the default), the result is multiplied by time duration or monitor count of each rotation.
The corresponding error variances are scaled consistently.

Uncertainties
-------------

The percentile is an estimator built from the rotation steps that contribute to it, not a single
measurement, so it is more precise than the value it selects. For :math:`n` contributing rotations
whose values have standard deviation :math:`\sigma`, the output variance is

.. math::

   \mathrm{Var}(\hat{q}_p) = \frac{p(1-p)}{\phi(z_p)^2} \frac{\sigma^2}{n}

where :math:`p` is ``BackgroundLevel`` expressed as a fraction, :math:`z_p` is the standard normal
quantile at :math:`p` and :math:`\phi` its density. The leading factor is :math:`\pi/2` for the
median, giving the familiar :math:`1.2533\,\sigma/\sqrt{n}`. Here :math:`n` is the length of the
rotation axis when ``BackgroundWindowSize`` is unset, and ``BackgroundWindowSize`` otherwise, while
:math:`\sigma^2` is the input variance of the value the percentile selected. Taking :math:`\sigma`
at the percentile rather than across all contributing rotations keeps the estimate free of the Bragg
peaks that the percentile is chosen to reject.

A ``BackgroundLevel`` of 0 or 100 selects the smallest or largest value, for which this limit does not
apply; those cases keep the variance of the selected value, which is a conservative upper bound.

Two limitations are worth noting. Where the sliding window is padded at the ends of an incomplete
rotation, it repeats values, so fewer than ``BackgroundWindowSize`` independent rotations contribute
and the variance is slightly underestimated. More importantly, the output uncertainties are strongly
correlated - completely so across the rotation axis when ``BackgroundWindowSize`` is unset, and
between neighbouring rotations otherwise, since their windows overlap. A ``MDHistoWorkspace`` cannot
represent that correlation, so any subsequent operation that combines these values along the rotation
axis will underestimate the resulting uncertainty.



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
