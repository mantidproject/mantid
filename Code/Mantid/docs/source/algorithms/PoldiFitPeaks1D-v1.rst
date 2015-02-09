.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiFitPeaks1D takes a TableWorkspace with peaks (for example from :ref:`algm-PoldiPeakSearch`) and a spectrum from :ref:`algm-PoldiAutoCorrelation` and tries to fit a Gaussian peak profile to the spectrum for each peak. As described on the page of :ref:`algm-PoldiAutoCorrelation`, each peak is surrounded by a dip. This dip is modeled by adding a quadratic background of the form

    :math:`f(x) = a_0\cdot(x - x_0)^2 + a_1`

where :math:`x_0` is the peak position and :math:`a_i` are the coefficients of the polynome.

The implementation is very close to the original POLDI analysis software
(using the same profile function). One point where this routine differs
is error calculation. In the original program the parameter errors were
adjusted by averaging :math:`\chi^2`-values, but this does not work
properly if there is an outlier caused by a bad fit for one of the
peaks, so this step is not performed, the errors are derived directly from the fit.

The algorithm produces a number of output workspaces. One with refined peak parameters (OutputWorkspace), which is equivalent to the input TableWorkspace, except that the parameters are fitted and have errors. ResultTableWorkspace mimics the result files produced by the original data analysis software. FitCharacteristicsWorkspace contains the raw fitting results and the errors of the parameters, as well as :math:`\chi^2`-values for each profile fit. FitPlotsWorkspace is a GroupWorkspace that contains plots for each peak profile fit with measured data, fitted data and a difference curve.

Usage
-----

.. include:: ../usagedata-note.txt

The following small usage example performs a peak fit on the sample data already used in :ref:`algm-PoldiAutoCorrelation` and :ref:`algm-PoldiPeakSearch`. After the fit, the plots can be viewed and used to visually judge the quality of the fits.

.. testcode:: ExSiliconPeakFit

    # Load data file and instrument, perform correlation analysis
    raw_6904 = LoadSINQFile(Filename = "poldi2013n006904.hdf", Instrument = "POLDI")
    LoadInstrument(raw_6904, InstrumentName = "POLDI")
    correlated_6904 = PoldiAutoCorrelation(raw_6904)
    
    # Run peak search algorithm, store peaks in TableWorkspace
    peaks_6904 = PoldiPeakSearch(correlated_6904)
    
    PoldiFitPeaks1D(InputWorkspace = correlated_6904, FwhmMultiples = 4.0,
                    PeakFunction = "Gaussian", PoldiPeakTable = peaks_6904,
                    OutputWorkspace = "peaks_refined_6904",
                    FitPlotsWorkspace = "fit_plots_6904",
                    Version=1)
                    
    print "There are", mtd['fit_plots_6904'].getNumberOfEntries(), "plots available for inspection."
    
Output:

.. testoutput:: ExSiliconPeakFit

    There are 14 plots available for inspection.
    
.. categories::
