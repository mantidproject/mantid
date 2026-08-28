.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm integrates single crystal Bragg peaks by reusing the
ellipsoidal peak shape already stored on each peak in the input
*PeaksWorkspace*, instead of fitting a new shape from the events around each
peak. It sums the raw or weighted events inside that fixed ellipsoid and
subtracts a background estimate from the ellipsoidal shell already defined by
the shape's *BackgroundInnerSize* and *BackgroundOuterSize*.

The *PeaksWorkspace* must already have an ellipsoidal shape set on every peak
to be integrated, for example from a previous run of
:ref:`algm-IntegrateEllipsoids` or :ref:`algm-IntegrateEllipsoidsTwoStep`.
This makes the algorithm useful for re-integrating the same or a different
event workspace (e.g. a rebinned, background-subtracted, or otherwise
corrected dataset) without re-deriving the peak shapes each time, or for
comparing intensities obtained with a shape held fixed across multiple
datasets.

Peaks must be indexed with integral HKL values, in the same way as for
:ref:`algm-IntegrateEllipsoidsTwoStep`: the indexed peaks are used to
determine a :ref:`UB matrix <Lattice>`, whose inverse is used to assign each
event to the nearest peak in reciprocal space. Only events within
*RegionRadius* of a peak's Q-vector are considered; this should be at least as
large as the largest background outer radius among the peaks being
integrated, or the background shell will be truncated.

If *UseOnePercentBackgroundCorrection* is enabled (the default), the top 1% of
the background events are removed before background subtraction, to reduce
sensitivity to intensity spikes from nearby peaks.

Usage
-----

.. code-block:: python
   :linenos:

   # PeaksWorkspace already integrated once, so every peak has a shape
   IntegrateEllipsoids(InputWorkspace='TOPAZ_3132_event', PeaksWorkspace='TOPAZ_3132_peaks',
                       OutputWorkspace='TOPAZ_3132_peaks')

   # Reuse those shapes to integrate again, e.g. after correcting the events
   IntegratePeaksShapeMD(InputWorkspace='TOPAZ_3132_event', PeaksWorkspace='TOPAZ_3132_peaks',
                         RegionRadius='0.25', OutputWorkspace='TOPAZ_3132_peaks_reintegrated')

.. categories::

.. sourcelink::
