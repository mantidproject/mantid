.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm produces a fit-independent estimate of a peak's background and integrated intensity
over a window. The ``PeakWindowWorkspace`` gives per-spectrum windows following the
:ref:`algm-FitPeaks` ``FitPeakWindowWorkspace`` convention: one spectrum per ``InputWorkspace``
spectrum, each holding ``2*nPeaks`` X values arranged as ``[min0, max0, min1, max1, ...]``. Because
the window is read per spectrum, the same peak may occupy a different window in each spectrum (for
example a single d-spacing peak mapped onto each detector's TOF). A ragged ``PeakWindowWorkspace`` is
accepted: every spectrum must hold a non-zero even number of X values, but spectra may hold different
numbers of pairs, in which case the number of peaks is taken from the spectrum holding the most and a
spectrum holding fewer covers only the leading peak indices. For each peak and spectrum it:

#. resolves the integration window ``[min, max]`` to bin indices;
#. estimates the background as the mean of the "background" points selected by a *skew-seed* method:
   the points in the window are sorted by descending value and peeled off one at a time while the
   third central moment of the remaining points keeps decreasing and stays non-negative; the points
   left over are the background;
#. integrates ``(data - background)`` across the window with the trapezoidal rule.

The loop over spectra is parallelised with OpenMP, mirroring :ref:`algm-FitPeaks`. The estimate is
independent of any peak fit and is intended as a cross-check of a fitted peak area — it reproduces
the estimate previously computed in Python by the Engineering texture peak-fitting workflow.

The result is a :py:obj:`TableWorkspace <mantid.api.ITableWorkspace>` with one row per (peak,
spectrum), laid out peak-major, with the columns *PeakIndex*, *WorkspaceIndex*, *Intensity*,
*Sigma*, *Background* and *PeakCentre*. A spectrum whose window contains no positive data is reported
with zero *Intensity*, *Sigma* and *Background*, and its *PeakCentre* is the window midpoint. A peak
index beyond the number of pairs held by a given spectrum (only possible with a ragged
``PeakWindowWorkspace``) is likewise reported with zeros, but with a NaN *PeakCentre*, since there is
no window whose midpoint could be reported.

.. warning::

   *PeakIndex* is only the position of the ``[min, max]`` pair within that spectrum's window list. The
   algorithm never identifies peaks, so rows sharing a *PeakIndex* correspond to the same physical
   (d-spacing) peak **only if** the caller built ``PeakWindowWorkspace`` so that the n-th pair refers
   to the same peak in every spectrum. If the windows for a spectrum are ordered differently, or a
   spectrum omits a peak that another spectrum includes (shifting every later pair down by one), then
   grouping the table by *PeakIndex* mixes different peaks together. This is easy to hit with a ragged
   ``PeakWindowWorkspace``: to keep the indices aligned, give every spectrum the same number of pairs
   in the same peak order, and use a window containing no positive data (rather than dropping the
   pair) for a peak that is absent from a spectrum.

Usage
-----

**Example: estimate the intensity of a peak across two spectra.**

.. testcode:: ExEstimatePeakIntensities

    import numpy as np

    # two spectra sharing a common grid, each with a peak on a flat background
    x = np.arange(8.0)
    y = np.array([2, 2, 2, 10, 2, 2, 2,   2, 2, 2, 12, 2, 2, 2], dtype=float)
    ws = CreateWorkspace(DataX=np.tile(x, 2), DataY=y, DataE=np.ones_like(y), NSpec=2)

    # per-spectrum window [0.5, 6.5] for a single peak (2 X values per spectrum)
    win = CreateWorkspace(DataX=[0.5, 6.5, 0.5, 6.5], DataY=[0, 0], NSpec=2)
    table = EstimatePeakIntensities(InputWorkspace=ws, PeakWindowWorkspace=win)

    print("Background of spectrum 0 is {}".format(round(table.row(0)['Background'], 2)))
    print("Intensity of spectrum 0 is {}".format(round(table.row(0)['Intensity'], 2)))

Output:

.. testoutput:: ExEstimatePeakIntensities

    Background of spectrum 0 is 2.0
    Intensity of spectrum 0 is 8.0

.. categories::

.. sourcelink::
