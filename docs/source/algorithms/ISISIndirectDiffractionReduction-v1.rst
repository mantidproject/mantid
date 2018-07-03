.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The generic routine used to reduce diffraction runs from indirect geometry
inelastic instruments at ISIS.

Workflow
--------

.. diagram:: ISISIndirectDiffractionReduction-v1_wkflw.dot

Usage
-----

**Example - Running ISISIndirectDiffractionReduction.**

.. testcode:: ExISISIndirectDiffractionReductionSimple

    ISISIndirectDiffractionReduction(InputFiles='IRS21360.raw',
                                     OutputWorkspace='DiffractionReductions',
                                     Instrument='IRIS',
                                     Mode='diffspec',
                                     SpectraRange=[105,112])

    ws = mtd['DiffractionReductions'].getItem(0)

    print('Workspace name: {}'.format(ws.name()))
    print('Number of spectra: {:d}'.format(ws.getNumberHistograms()))
    print('Number of bins: {}'.format(ws.blocksize()))

Output:

.. testoutput:: ExISISIndirectDiffractionReductionSimple

    Workspace name: iris21360_diffspec_red
    Number of spectra: 1
    Number of bins: 1934

.. categories::

.. sourcelink::
