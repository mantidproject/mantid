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

Peaks need not be indexed: each event is assigned to the peak whose Q-vector
is nearest to it, so satellite and unindexed peaks are integrated as any
other. Only events within *RegionRadius* of a peak's Q-vector are
considered; this should be at least as large as the largest background outer
radius among the peaks being integrated, or the background shell will be
truncated.

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
peak's stored Q -- a slight correction, not a free centroid search. The
correction is written to the translation of the output peak's shape; the
peak's stored Q is not changed.

Events from masked detectors are not integrated.

Flux and solid angle normalization
##################################

If *SolidAngleWorkspace* and *FluxWorkspace* are given, each peak's
intensity is normalized by the flux and solid angle, in the same units as
summing the normalized signal of :ref:`algm-MDNorm` times the voxel volume
(in :math:`\mathrm{\AA}^{-3}`) over the peak, but without binning. The two
workspaces are the same as for :ref:`algm-MDNorm`: integrated vanadium for
the solid angle of each detector, and the integrated flux as a function of
momentum. As for :ref:`algm-MDNorm`, the *InputWorkspace* must first be
cropped with :ref:`algm-CropWorkspaceForMDNorm`, whose ``MDNorm_low`` and
``MDNorm_high`` logs give the momentum range measured by each detector.

Each detector pixel :math:`p` measures along the line
:math:`\mathbf{Q}(k) = k\,\mathbf{u}_p` in :math:`Q_\mathrm{lab}`, for
:math:`k` between ``MDNorm_low`` and ``MDNorm_high``, where
:math:`\mathbf{u}_p = \hat{\mathbf{k}}_i - \hat{\mathbf{k}}_{f,p}` (with the
opposite sign for the Crystallography ``Q.convention``). The line crosses an
ellipsoid :math:`(\mathbf{Q} - \mathbf{c})^T M (\mathbf{Q} - \mathbf{c}) = 1`
between the roots :math:`k_1, k_2` of
:math:`a k^2 - 2 b k + d = 0`, with :math:`a = \mathbf{u}_p^T M \mathbf{u}_p`,
:math:`b = \mathbf{u}_p^T M \mathbf{c}` and
:math:`d = \mathbf{c}^T M \mathbf{c} - 1`, so the normalization of the
ellipsoid is

.. math::

   \mathcal{N} = q \sum_p \Omega_p \left[\Phi_p(k_2) - \Phi_p(k_1)\right],

where :math:`\Omega_p` is the solid angle, :math:`\Phi_p` the integrated
flux (interpolated as :ref:`algm-MDNorm` does) and :math:`q` the proton
charge, with :math:`k_1, k_2` clipped to the pixel's momentum range. This is
what :ref:`algm-MDNorm` sums over the voxels inside the ellipsoid, computed
exactly for the ellipsoid instead of for voxels. Monitors, masked detectors
and detectors missing from either workspace are left out.

In count mode, the intensity is the background-subtracted mean normalized
signal over the peak ellipsoid times its volume :math:`V`,

.. math::

   I = V \left(\frac{N_\mathrm{peak}}{\mathcal{N}_\mathrm{peak}} -
   \frac{N_\mathrm{shell}}{\mathcal{N}_\mathrm{shell}}\right),

with :math:`N` the counts and :math:`\mathcal{N}` the normalization of the
peak ellipsoid and of the background shell, and its uncertainty is
propagated from the counts alone. *UseOnePercentBackgroundCorrection* is not
used. Count mode is a ratio of sums: it divides the counts by the mean
exposure over the ellipsoid rather than by the exposure where the peak is,
so it equals the binned :ref:`algm-MDNorm` result only when the exposure is
uniform across the ellipsoid. The exposure falls off roughly as
:math:`1/k^2`, so this differs from the binned result by a few percent for
large ellipsoids, and much more for peaks only partly covered by the
detectors, for example by masked detectors or at the edge of a detector or
of the momentum range.

With *ProfileFit*, the normalization enters the expected counts of the
likelihood instead. For a Gaussian peak :math:`A\,g(\mathbf{Q})` (of peak
height :math:`A`) on a flat background :math:`B`,

.. math::

   \ln L = \sum_i w_i \ln\left(A\,g(\mathbf{Q}_i) + B\right)
   - A\,G(\mathbf{c}) - B\,\mathcal{N}_\mathrm{region},
   \qquad
   G(\mathbf{c}) = q \sum_p \Omega_p \int g(k\,\mathbf{u}_p)\,d\Phi_p(k),

where the sum runs over the events and the integral over each pixel's chord
through the fit region, a sphere of *RegionRadius* about the peak, whose
normalization is :math:`\mathcal{N}_\mathrm{region}`. Along each line the
Gaussian is a Gaussian in :math:`k`, and :math:`\Phi_p` is linear between its
points, so the integral is evaluated exactly. The intensity is
:math:`I = (2\pi)^{3/2} \sigma_a \sigma_b \sigma_c A`, the integral of the
fitted peak, and so it accounts for the exposure across the peak, including
peaks only partly covered by the detectors. With *AdjustCenter*, the
gradient of :math:`G` with respect to the center is included in the
center correction.

With *AdjustShape*, the shape of each peak whose intensity (fitted with its
stored shape) is at least *AdjustShapeMinSignalToNoise* standard deviations
is refined too: the likelihood is maximized over the covariance of a
normalized Gaussian (and with *AdjustCenter* its center) by damped
Gauss-Newton steps, each kept only if it increases the likelihood, with each
principal standard deviation kept within a factor *MaxShapeChange* of the
stored shape's (in the stored shape's own frame). With normalization the
expected counts depend on the shape through :math:`G`, whose gradient with
respect to the inverse covariance is
:math:`-\tfrac{1}{2} q \sum_p \Omega_p \int (k\mathbf{u}_p - \mathbf{c})
(k\mathbf{u}_p - \mathbf{c})^T g\,d\Phi_p`, also evaluated exactly, so the
refined shape is that of the intensity rather than of the counts, which the
exposure distorts. The refined shape is written to the output peak, with its
background radii scaled with it, and the uncertainty of a peak with a refined
shape includes that of its shape. Weaker peaks keep their stored shape, as
does a peak whose refined intensity, with that larger uncertainty, falls
below *AdjustShapeMinSignalToNoise*. With *AdjustCenter* too, the center is
refined only together with the shape, so a peak that keeps its stored shape
also keeps its stored center. The refined center may move up to
*MaxCenterShift* standard deviations of the stored shape (in its own frame)
from the stored center, as a predicted position can be several standard
deviations off along a narrow axis, e.g. from an imperfect calibration. Such
a peak is out of reach of the local Gauss-Newton steps, and looks weaker at
its stored center than it is, so the fit, and the significance test, start
from the mode of the counts smoothed by the stored shape (found by mean
shift, first with a kernel twice as wide) if the peak is more significant
there.

The events in the fit region are those nearest to the peak in reciprocal
space, so *RegionRadius* should be less than half the shortest distance
between peaks for the fit region to be the sphere assumed above. The
uncertainty of the vanadium is not propagated.

*OutputDiagnosticsWorkspace* gives one row per peak, with the counts and
normalization in the peak ellipsoid (*CountsPeak*, *NormPeak*) and the
background shell (*CountsShell*, *NormShell*), the number of pixels whose
lines cross the peak ellipsoid (*NumPixels*), and whether any pixel's line
through the ellipsoids lies partly outside its momentum range
(*ClippedByMomentumRange*) or any masked detector's line crosses them
(*MaskedDetectors*), which flag peaks whose count mode intensities may be
biased, whether the peak's shape was refined (*ShapeAdjusted*), and the
time spent integrating it (*FitSeconds*). A summary of the time spent
gathering events, setting up the normalization and integrating the peaks
is logged. With *ProfileFit*, *CountsPeak* is the summed event weight in the
fit region, *CountsShell* the fitted background counts, *NormPeak* the
expected counts per unit intensity of the fitted peak (so that
:math:`I \cdot \mathit{NormPeak} + \mathit{CountsShell}` is the expected
count in the fit region), *NormShell* the normalization of the fit region,
and the pixel count and flags refer to the fit region.

The normalization is for a single time-of-flight run; monochromatic data,
several runs at once, symmetry-equivalent merging and sloped backgrounds
are not supported.

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

**Example - normalizing by flux and solid angle:**

Normally the solid angle and flux come from a vanadium run, as for
:ref:`algm-MDNorm`. To keep this example small, a uniform solid angle and a
flat flux stand in for them.

.. testcode:: exNormalized

   import numpy as np

   ws = Load(Filename='TOPAZ_36079_crop.nxs', FilterByTofMin=500, FilterByTofMax=16666)
   LoadInstrument(Workspace=ws, InstrumentName='TOPAZ', RewriteSpectraMap=False)
   LoadIsawDetCal(InputWorkspace=ws, Filename='TOPAZ_2020A.DetCal')

   # Bragg peaks from a known UB, with shapes from IntegrateEllipsoids
   UB = np.array([[0.15468228, 0.10908475, -0.14428671],
                  [-0.08922105, -0.08617147, -0.22976459],
                  [-0.05616441, 0.12536522, -0.03238277]])
   peaks = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0)
   SetUB(peaks, UB=UB)
   for hkl in [[0, 2, 1], [0, 4, 2], [1, 5, -3], [1, 4, -3]]:
       AddPeakHKL(peaks, hkl)
   peaks = IntegrateEllipsoids(InputWorkspace=ws, PeaksWorkspace=peaks, RegionRadius=0.055,
                               SpecifySize=True, PeakSize=0.0425, BackgroundInnerSize=0.043,
                               BackgroundOuterSize=0.055)

   # crop to a momentum band, recorded in the MDNorm_low and MDNorm_high logs
   ws = ConvertUnits(ws, Target='Momentum')
   ws = CropWorkspaceForMDNorm(ws, XMin=1.8, XMax=12.5)

   # a uniform solid angle for every detector, and a flat integrated flux
   solid_angle = Rebin(ws, Params='1.8,10.7,12.5', PreserveEvents=False)
   for i in range(solid_angle.getNumberHistograms()):
       solid_angle.setY(i, np.ones(1))
   summed = SumSpectra(ws)
   spectrum = Rebin(summed, Params='1.8,0.01,12.5', PreserveEvents=False)
   spectrum.setY(0, np.ones(spectrum.blocksize()))
   flux = IntegrateFlux(spectrum)

   normalized, diagnostics = IntegratePeaksShapeMD(InputWorkspace=ws, PeaksWorkspace=peaks, RegionRadius=0.055,
                                                   SolidAngleWorkspace=solid_angle, FluxWorkspace=flux,
                                                   OutputDiagnosticsWorkspace='diagnostics')
   print('Integrated {} peaks'.format(normalized.getNumberPeaks()))
   print('Every peak covered by detector pixels: {}'.format(all(n > 0 for n in diagnostics.column('NumPixels'))))
   print('Any partly covered: {}'.format(any(diagnostics.column('ClippedByMomentumRange'))))

Output:

.. testoutput:: exNormalized

   Integrated 4 peaks
   Every peak covered by detector pixels: True
   Any partly covered: False

.. categories::

.. sourcelink::
