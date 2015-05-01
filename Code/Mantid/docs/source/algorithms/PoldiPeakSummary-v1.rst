
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes the peak table resulting from one of the POLDI peak fitting routines (for example :ref:`algm-PoldiFitPeaks1D`) and summarizes the data in another table with the relevant information.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - PoldiPeakSummary**

.. testcode:: PoldiPeakSummaryExample

    # Load data file and instrument, perform correlation analysis
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, InstrumentName = "POLDI")
    correlated_6904 = PoldiAutoCorrelation(raw_6904)
    
    # Run peak search algorithm, store peaks in TableWorkspace
    peaks_6904 = PoldiPeakSearch(correlated_6904)
    
    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904",
                    FitPlotsWorkspace = "fit_plots_6904")
                    
    summary_6904 = PoldiPeakSummary(mtd["peaks_refined_6904"])
    
    print "Number of refined peaks:", summary_6904.rowCount()
    print "Number of columns that describe a peak:", summary_6904.columnCount()

Output:

.. testoutput:: PoldiPeakSummaryExample

    Number of refined peaks: 13
    Number of columns that describe a peak: 6

.. categories::

