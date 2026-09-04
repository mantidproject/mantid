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
the shape's own background inner and outer radii -- unlike
:ref:`algm-IntegrateEllipsoidsTwoStep`, this algorithm has no *PeakSize*,
*BackgroundInnerSize* or *BackgroundOuterSize* properties of its own; every
peak's ellipsoid is used exactly as stored. This also differs from
:ref:`algm-IntegratePeaksMD`, which always integrates with a spherical
(not ellipsoidal) region computed from user-specified radii rather than a
shape stored on the peak.

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

Integration method
###################

By default (*ProfileFit* = False) each peak is integrated by counting raw
or weighted events inside the peak ellipsoid, and subtracting a background
estimate from the ellipsoidal shell, the same background-subtraction method
:ref:`algm-IntegrateEllipsoidsTwoStep` uses (based on the ILL program Racer
and Wilkinson, C., et al. "Integration of single-crystal reflections using
area multidetectors." *Journal of Applied Crystallography* 21.5 (1988):
471-478) -- applied here to a supplied, rather than fitted, ellipsoid.

If *ProfileFit* is enabled, each peak is instead integrated by directly
maximizing the (weighted) Poisson log-likelihood of an unbinned point
process, fitting a Gaussian peak amplitude and a flat background rate
against the raw events within *RegionRadius* of the peak. In this mode the
peak radii are interpreted as the Gaussian's standard deviations (1-sigma)
along its principal axes rather than as hard integration boundaries, and
the background radii are not used since the background rate is fit
directly instead. This can make better use of the available events for weak
peaks than a simple ellipsoidal count, at the cost of assuming the peak
profile is well described by a Gaussian and that essentially all of its
intensity falls within *RegionRadius*.

Any scaling or mosaic-broadening correction to a peak's shape (e.g. derived
from a resolution model, or refined per-sample) is expected to be applied
by the caller before running this algorithm, by writing the corrected
radii directly into the *PeaksWorkspace*'s stored shapes -- this algorithm
always uses the shape it is given exactly as supplied, with no adjustment
of its own.

By default the peak center used is always the peak's own stored Q-vector;
neither integration method refines or re-centers it, matching
:ref:`algm-IntegrateEllipsoidsTwoStep`, which also never adjusts a peak's
center (weak peaks borrow a strong peak's *shape*, but stay centered at
their own stored Q). If *ProfileFit* and *AdjustCenter* are both enabled,
the center is additionally refined by a bounded Gauss-Newton correction as
part of the same fit, capped at one standard deviation of shift from the
peak's stored Q -- a slight correction, not a free centroid search. This
correction is only used for this integration; it is not written back to
the peak's stored position.

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
