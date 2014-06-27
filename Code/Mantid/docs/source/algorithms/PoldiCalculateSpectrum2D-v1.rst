.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiCalculateSpectrum2D is a preliminary algorithm that can be used to calculate 2D POLDI-data from a set of peaks. These must come in a table of special format, which may be generated for example by :ref:`algm-PoldiFitPeaks1D`. Furthermore, the algorithm needs a MatrixWorkspace containing raw POLDI data with correct dimensions and a proper instrument definition.

At the moment all profiles are calculated independently, using Gaussian functions. In future versions of the algorithm this will be much more flexible, including background functions.

Usage
-----

.. include:: ../usagedata-note.txt

PoldiAutoCorrelation operates on a MatrixWorkspace with a valid POLDI instrument definition. The following short example demonstrates how to use the algorithm, processing data obtained from recording the spectrum of a Silicon standard material (powder).

.. testcode:: ExSilicon2D

    # Load data file with Si spectrum and instrument definition
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, InstrumentName = "POLDI")
    
    # Data needs to be truncated to correct dimensions.
    truncated_6904 = PoldiTruncateData(raw_6904)
    
    # Perform correlation, peak search and fit
    correlated_6904 = PoldiAutoCorrelation(truncated_6904)
    peaks_6904 = PoldiPeakSearch(correlated_6904)
    
    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904", ResultTableWorkspace = "result_table_6904",
                    FitCharacteristicsWorkspace = "raw_fit_results_6904",
                    FitPlotsWorkspace = "fit_plots_6904")
                    
    # Calculate a 2D spectrum using the refined peaks
    PoldiCalculateSpectrum2D(InputWorkspace=truncated_6904,
                             PoldiPeakWorkspace="peaks_refined_6904",
                             OutputWorkspace="simulated_6904")
    
After this step, there is a new workspace containing the simulated spectrum. It should look similar to the one in the following figure:

.. figure:: /images/PoldiAutoCorrelation_Si_2D.png
   :figwidth: 15 cm
   :align: center
   :alt: Raw POLDI data for Silicon powder standard (simulated).
   
   Simulated 2D-spectrum of silicon powder.

.. categories::
