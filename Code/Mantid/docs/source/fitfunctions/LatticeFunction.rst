.. _func-LatticeFunction:

===============
LatticeFunction
===============

.. index:: LatticeFunction

Description
-----------

After a list of Bragg reflections has been indexed, it is often necessary to refine the unit cell parameters that have
been used to assign indices. LatticeFunction can be used to achieve that task with :ref:`algm-Fit`. The function can
work with a PeaksWorkspace or with a TableWorkspace that contains two columns named `HKL` and `d` (see
:ref:`algm-PawleyFit` for specification of the table).

After setting the `CrystalSystem` attribute to one of the seven crystal systems, the function exposes the
corresponding lattice parameters, as well as a `ZeroShift`. In most cases it's recommended to fix this additional
parameter as this is often taken care of in earlier steps of the data reduction.

Usage
-----

.. include:: ../usagedata-note.txt

The following script demonstrates how the function can be used. The algorithm :ref:`algm-PoldiCreatePeaksFromCell` is
used to generate Bragg reflections that are expected for the crystal structure of Silicon.

.. testcode:: ExSiliconTheoretical

    import numpy as np

    # Create Silicon peaks for indexing
    peaks_Si = PoldiCreatePeaksFromCell(
                SpaceGroup="F d -3 m",
                a=5.4311946,
                Atoms="Si 0 0 0",
                LatticeSpacingMin=0.7)

    # Fit a cubic cell, starting parameter is 5
    Fit(Function="name=LatticeFunction,CrystalSystem=Cubic,a=5",
        InputWorkspace=peaks_Si,
        Ties="ZeroShift=0.0",
        CreateOutput=True,
        Output="Si",
        CostFunction="Unweighted least squares")

    # Print the refined lattice parameter with error estimate
    parameters = AnalysisDataService.retrieve("Si_Parameters")

    a_true = 5.4311946
    a = np.round(parameters.cell(0, 1), 7)
    a_err = np.round(parameters.cell(0, 2), 7)

    print "Refined lattice parameter: a =", a, "+/-", a_err
    print "Difference from expected value: a_observed - a_expected =", np.round(a - a_true, 7)
    print "Is this difference within the standard deviation?", "Yes" if np.fabs(a - a_true) < a_err else "No"

Executing the script produces some output with information about the fit:

.. testoutput:: ExSiliconTheoretical

    Refined lattice parameter: a = 5.4311944 +/- 3e-07
    Difference from expected value: a_observed - a_expected = -2e-07
    Is this difference within the standard deviation? Yes

In addition there is also an output workspace, which contains information about the peaks used for the fit and how
well the peak positions calculated from the fitted parameters match the observed positions.

.. testcleanup:: ExSiliconTheoretical

    DeleteWorkspace("peaks_Si")
    DeleteWorkspace("Si_Parameters")
    DeleteWorkspace("Si_Workspace")
    DeleteWorkspace("Si_NormalisedCovarianceMatrix")

.. attributes::

.. properties::

.. categories::
