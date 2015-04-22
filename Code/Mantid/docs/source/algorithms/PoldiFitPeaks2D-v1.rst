.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiFitPeaks2D is an algorithm that can be used to fit a set of individual peaks to 2D POLDI-data. These must come in a table of special format, which may be generated for example by :ref:`algm-PoldiFitPeaks1D`. Furthermore, the algorithm needs a MatrixWorkspace containing raw POLDI data with correct dimensions and a proper instrument definition.

The 1D-peak intensities need to be integral intensities, so the peaks are integrated if necessary. If there is no profile information supplied in the peak table (:ref:`algm-PoldiFitPeaks1D` adds this automatically), it's possible to supply a profile function as parameter to this algorithm. If a profile function name is present in the peak table, the one supplied in the parameters has priority.

There are two modes for performing the fit. In the default mode, all peak profiles are fitted independently using the same function that is used for integration. The other possibility is to perform a Pawley-type fit, where peak positions are calculated using lattice parameters and Miller indices (see :ref:`algm-PawleyFit` for a more general explanation of the method). This mode is controlled by the PawleyFit parameter, if it is activated, InitialCell and CrystalSystem need to be specified as well. For these fits, an additional table will be created which contains the refined lattice parameters. Please note that the peaks need to be indexed to use this mode (:ref:`algm-PoldiCreatePeaksFromCell`, :ref:`algm-PoldiIndexKnownCompounds`).

PoldiFitPeaks2D can also be used to calculate a theoretical 2D pattern from a set of peaks by limiting the iterations to 0.

In addition to performing the 2D-fit, a theoretical 1D-diffractogram of the fit-function is calculated as well, which can be used in conjunction with :ref:`algm-PoldiAnalyseResiduals` to assess the quality of a fit.

Usage
-----

.. include:: ../usagedata-note.txt

**Individual peak profiles**

PoldiFitPeaks2D operates on a MatrixWorkspace with a valid POLDI instrument definition. The following short example demonstrates how to use the algorithm, processing data obtained from recording the spectrum of a Silicon standard material (powder) and calculating a theoretical 2D-spectrum.

.. testcode:: ExSilicon2D

    # Load data file with Si spectrum and instrument definition
    truncated = PoldiLoadRuns(2013, 6904)
    
    # Perform correlation, peak search and fit
    correlated_6904 = PoldiAutoCorrelation("truncated_data_6904")
    peaks_6904 = PoldiPeakSearch(correlated_6904)

    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904",
                    FitPlotsWorkspace = "fit_plots_6904")
                    
    # Calculate a 2D spectrum using the refined peaks
    PoldiFitPeaks2D(InputWorkspace="truncated_data_6904",
                                PoldiPeakWorkspace="peaks_refined_6904",
                                RefinedPoldiPeakWorkspace="peaks_fit_2d_6904",
                                Calculated1DSpectrum="simulated_1d_6904",
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
    truncated = PoldiLoadRuns(2013, 6904)

    # Perform correlation, peak search and fit
    correlated_6904 = PoldiAutoCorrelation("truncated_data_6904")
    peaks_6904 = PoldiPeakSearch(correlated_6904)

    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904",
                    FitPlotsWorkspace = "fit_plots_6904")

    # Calculate a 2D spectrum using the refined peaks - with background linear in 2theta
    PoldiFitPeaks2D(InputWorkspace="truncated_data_6904",
                             PoldiPeakWorkspace="peaks_refined_6904",
                             OutputWorkspace="simulated_6904",
                             RefinedPoldiPeakWorkspace="peaks_fit_2d_6904",
                             Calculated1DSpectrum="simulated_1d_6904",
                             LinearBackgroundParameter=0.01)

Now the spectrum looks different, like in the example below.

.. figure:: /images/PoldiAutoCorrelation_Si_2D_bg.png
   :figwidth: 15 cm
   :align: center
   :alt: Raw POLDI data for Silicon powder standard with background (simulated).
   
   Simulated 2D-spectrum of silicon powder with background.

Furthermore, a 1D diffractogram is also calculated, which shows all peaks that were used to calculate the 2D spectrum as well.

.. figure:: /images/PoldiFitPeaks2D_Si_1D.png
   :figwidth: 15 cm
   :align: center
   :alt: Calculated diffractogram for Silicon powder standard.

   Calculated diffractogram for Silicon powder standard.

**Pawley-type fit**

The following example shows an example for refinement of lattice parameters using the PawleyFit-option.

.. testcode:: ExSilicon2DPawley

    import numpy as np

    # Load and merge 2 data files for better statistics.
    truncated = PoldiLoadRuns(2013, 6903, 6904, 2)
    
    # Perform correlation, peak search and fit
    correlated_6904 = PoldiAutoCorrelation("truncated_data_6904")
    peaks_6904 = PoldiPeakSearch(correlated_6904)

    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904",
                    FitPlotsWorkspace = "fit_plots_6904")

    # Generate reflections for Silicon
    si_peaks = PoldiCreatePeaksFromCell(SpaceGroup = "F d -3 m",
                                        Atoms = "Si 0.0 0.0 0.0",
                                        a = 5.431,
                                        LatticeSpacingMin = 0.7)
    # Index the refined peaks
    indexed = PoldiIndexKnownCompounds("peaks_refined_6904",
                                       CompoundWorkspaces = "si_peaks")

    # Only consider the first 8 peaks
    DeleteTableRows("indexed_si_peaks", "8-30")

    # Fit a unit cell.
    PoldiFitPeaks2D(InputWorkspace="truncated_data_6904",
                             PoldiPeakWorkspace="indexed_si_peaks",
                             OutputWorkspace="fitted_6904",
                             PawleyFit = True,
                             InitialCell = "5.431 5.431 5.431 90 90 90",
                             CrystalSystem = "Cubic",
                             MaximumIterations=100,
                             RefinedPoldiPeakWorkspace="peaks_fit_2d_6904",
                             Calculated1DSpectrum="simulated_1d_6904",
                             RefinedCellParameters="refined_cell_6904")


    lattice_parameters = AnalysisDataService.retrieve("refined_cell_6904")

    cell_a = np.round(lattice_parameters.cell(0, 1), 5)
    cell_a_error = np.round(lattice_parameters.cell(0, 2), 5)

    print "Refined lattice parameter a =", cell_a, "+/-", cell_a_error

The refined lattice parameter is printed at the end:

.. testoutput:: ExSilicon2DPawley

    Refined lattice parameter a = 5.43125 +/- 4e-05

.. categories::
