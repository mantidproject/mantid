.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

PoldiPeakSearch is a peak-finding routine for POLDI correlation data obtained with :ref:`algm-PoldiAutoCorrelation`. These spectra are often very spikey, so identifying peaks can be difficult, especially in the low :math:`Q`-region of the spectrum. The original analysis software implemented a special procedure to avoid these problems and this implementation is very close to it.

The algorithm
#############

1. Map each point of the spectrum :math:`y`, except the first and the
   last to the sum of its value and its neighbor's values:

    :math:`y'_i = y_{i-1} + y_{i} + y_{i+1}`

   The new spectrum :math:`y'` contains :math:`n-2` points when :math:`y` contains :math:`n`.

#. Identify peak positions in :math:`y'`, which is done with a recursive
   algorithm, consisting of these steps:

   a. Find the position of the maximum, :math:`i_{max}` in the list,
      store in peak-list.
   #. Split the list in two parts,
      :math:`[i_{0} + \Delta, i_{max} - \Delta)` and :math:`(i_{max} + \Delta, i_{n} - \Delta]`,
      where :math:`\Delta` is the minimum number of data points between two peaks.
   #. If ranges are valid, perform algorithm on each of the sublists,
      append returned lists to peak-list.
   #. Return peak-list.

#. Sort list by value (summed correlation counts, :math:`y'`) in descending order, keep the first
   :math:`N_{max}` items of the list.
#. Map peak positions from :math:`y'` back to :math:`y`
#. Perform background and fluctuation estimation:

   a. Extract all points from :math:`y` (except the first and the last)
      that are further than :math:`\Delta` elements away from any peak
      position
   #. Calculate median of these points as location estimate
      (:math:`\bar{b}`)
   #. Calculate :math:`S_n` as scale estimator (:math:`\bar{s}`)

#. Estimate peak intensity as :math:`y_{i}`
#. If a minimum peak height is set, discard all peaks that are smaller
   than this, if not, discard all peaks that are lower than
   :math:`3\cdot\bar{s} + \bar{b}`

The peaks are stored in a new table workspace.

Usage
-----

.. include:: ../usagedata-note.txt

A typical peak search procedure would be performed on correlation data, so this analysis is performed first, followed by a peak search with default parameters.

.. testcode:: ExSiliconPeakSearch

    # Load data file and instrument, perform correlation analysis
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, RewriteSpectraMap=True, InstrumentName = "POLDI")
    correlated_6904 = PoldiAutoCorrelation(raw_6904, Version=5)

    # Run peak search algorithm, store peaks in TableWorkspace
    peaks_6904 = PoldiPeakSearch(correlated_6904)

    # The tableworkspace should contain 14 peaks.
    print("The correlation spectrum of sample 6904 contains {} peaks.".format(peaks_6904.rowCount()))

Output:

.. testoutput:: ExSiliconPeakSearch

    The correlation spectrum of sample 6904 contains 14 peaks.

.. categories::

.. sourcelink::
