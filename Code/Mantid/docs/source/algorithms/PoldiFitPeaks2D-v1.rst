.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiFitPeaks2D is an algorithm that can be used to fit a set of individual peaks to 2D POLDI-data. These must come in a table of special format, which may be generated for example by :ref:`algm-PoldiFitPeaks1D`. Furthermore, the algorithm needs a MatrixWorkspace containing raw POLDI data with correct dimensions and a proper instrument definition.

The 1D-peak intensities need to be integral intensities, so the peaks are integrated if necessary. If there is no profile information supplied in the peak table (:ref:`algm-PoldiFitPeaks1D` adds this automatically), it's possible to supply a profile function as parameter to this algorithm. If a profile function name is present in the peak table, the one supplied in the parameters has priority.

At the moment all profiles are calculated independently, using Gaussian functions. In future versions of the algorithm this will be much more flexible.

PoldiFitPeaks2D can also be used to calculate a theoretical 2D pattern from a set of peaks by limiting the iterations to 0.

Usage
-----

.. include:: ../usagedata-note.txt

PoldiFitPeaks2D operates on a MatrixWorkspace with a valid POLDI instrument definition. The following short example demonstrates how to use the algorithm, processing data obtained from recording the spectrum of a Silicon standard material (powder) and calculating a theoretical 2D-spectrum.

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
    PoldiFitPeaks2D(InputWorkspace=truncated_6904,
                                PoldiPeakWorkspace="peaks_refined_6904",
                                RefinedPoldiPeakWorkspace="peaks_fit_2d_6904",
                                OutputWorkspace="simulated_6904")
    
After this step, there is a new workspace containing the simulated spectrum. It should look similar to the one in the following figure:

.. figure:: /images/PoldiAutoCorrelation_Si_2D.png
   :figwidth: 15 cm
   :align: center
   :alt: Raw POLDI data for Silicon powder standard (simulated).
   
   Simulated 2D-spectrum of silicon powder.

In general, there is a background in POLDI data that depends on :math:`2\theta`. The following script, which is almost identical to the above one introduces this parameter.

.. testcode:: ExSilicon2DBackground

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

    # Calculate a 2D spectrum using the refined peaks - with background linear in 2theta
    PoldiFitPeaks2D(InputWorkspace=truncated_6904,
                             PoldiPeakWorkspace="peaks_refined_6904",
                             OutputWorkspace="simulated_6904",
                             RefinedPoldiPeakWorkspace="peaks_fit_2d_6904",
                             LinearBackgroundParameter=0.01)

Now the spectrum looks different, like in the example below.

.. figure:: /images/PoldiAutoCorrelation_Si_2D_bg.png
   :figwidth: 15 cm
   :align: center
   :alt: Raw POLDI data for Silicon powder standard with background (simulated).
   
   Simulated 2D-spectrum of silicon powder with background.

.. categories::
