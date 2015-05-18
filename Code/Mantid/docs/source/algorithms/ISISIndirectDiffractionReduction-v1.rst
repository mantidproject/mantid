.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The generic routine used to reduce diffraction runs from indirect geometry
inelastic instruments at ISIS.

Workflow
--------

The workflow of this algoirhm is shown in the following flowchart.

.. diagram:: ISISIndirectDiffractionReduction-v1_wkflw.dot

This workflow uses routines from the IndirectReductionCommon Python file which
are described in the following flowchart.

.. diagram:: IndirectReductionCommon_wkflw.dot

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

    print 'Workspace name: %s' % ws.getName()
    print 'Number of spectra: %d' % ws.getNumberHistograms()
    print 'Number of bins: %s' % ws.blocksize()

Output:

.. testoutput:: ExISISIndirectDiffractionReductionSimple

    Workspace name: irs21360_diffspec_red
    Number of spectra: 1
    Number of bins: 1935

.. categories::
