.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs all necessary steps for a complete analysis of POLDI data, combining all algorithms that are
specific to POLDI. In detail, it performs these steps on the supplied data:

1. :ref:`algm-PoldiAutoCorrelation`
2. :ref:`algm-PoldiPeakSearch`
3. :ref:`algm-PoldiFitPeaks1D`
4. :ref:`algm-PoldiIndexKnownCompounds`
5. :ref:`algm-PoldiFitPeaks2D`
6. :ref:`algm-PoldiAnalyseResiduals`

Finally, the calculated diffractogram and the residuals are added together and all three spectra are plotted to give
an impression of the result. If the `MultipleRuns` option is activated, steps 2 - 6 are repeated, but instead of
using the initial correlation spectrum, the sum or calculated and residuals is used. Because this is usually much
smoother than the initial correlation spectrum, additional peaks can be found there sometimes. The 2D-fit is still
performed with the original data.

The actual output is a WorkspaceGroup with content that varies a bit depending on the input parameters. If
`PawleyFit` was active, it contains refined cell parameters. For the `ExpectedPeaks` parameter, a WorkpsaceGroup may
be given (this is necessary when multiple phases are present in a sample).

Usage
-----

.. include:: ../usagedata-note.txt

The algorithm requires relatively little input and can be run like this:

.. testcode::

    import numpy as np

    # Load POLDI data
    PoldiLoadRuns(2013, 6903, 6904, 2, OutputWorkspace='poldi', MaskBadDetectors=False)

    # Create Silicon peaks
    PoldiCreatePeaksFromCell(SpaceGroup='F d -3 m', Atoms='Si 0 0 0 1.0 0.01', a=5.431,
                             LatticeSpacingMin=0.7,
                             OutputWorkspace='Si')

    PoldiDataAnalysis(InputWorkspace='poldi_data_6904',
                      ExpectedPeaks='Si', PawleyFit=True,
                      MaximumPeakNumber=8,
                      PlotResult=False,
                      OutputWorkspace='result')

    # Take a look at the refined cell:

    cell = AnalysisDataService.retrieve('poldi_data_6904_cell_refined')

    cell_a = np.round(cell.cell(0, 1), 5)
    cell_a_error = np.round(cell.cell(0, 2), 5)

    print "Refined lattice parameter a =", cell_a, "+/-", cell_a_error

.. testoutput::

    Refined lattice parameter a = 5.43126 +/- 5e-05


.. categories::
