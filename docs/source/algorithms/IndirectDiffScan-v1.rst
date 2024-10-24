.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algortithm is to speed up the workflow for Indirect Inelastic diffraction reduction.
Uses :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` in diffspec mode with several parameters at fixed values.
Also extracts the temperature from each run and creates a workspace of all runs with respect to temperature.

Usage
-----

**Example - Running IndirectDiffScan.**

.. testcode:: ExIndirectDiffScan

    ISISIndirectDiffractionReduction(InputFiles='IRS21360.raw',
                                     OutputWorkspace='DiffractionReductions',
                                     Instrument='IRIS',
                                     SpectraRange=[105,112])

    ws = mtd['DiffractionReductions'].getItem(0)

    print('Workspace name: %s' % ws.name())
    print('Number of spectra: %d' % ws.getNumberHistograms())
    print('Number of bins: %s' % ws.blocksize())

Output:

.. testoutput:: ExIndirectDiffScan

    Workspace name: iris21360_diffspec_red
    Number of spectra: 1
    Number of bins: 1934

.. categories::

.. sourcelink::
