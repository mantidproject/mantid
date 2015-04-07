.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The generic routine used to reduce diffraction runs from indirect inelastic geometry instruments at ISIS.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Running MSGDiffractionReduction.**

.. testcode:: ExMSGDiffractionReductionSimple

    MSGDiffractionReduction(InputFiles='IRS21360.raw',
                            OutputWorkspace='DiffractionReductions',
                            Instrument='IRIS',
                            Mode='diffspec',
                            DetectorRange=[105,112])

    ws = mtd['DiffractionReductions'].getItem(0)

    print 'Workspace name: %s' % ws.getName()
    print 'Number of spectra: %d' % ws.getNumberHistograms()
    print 'Number of bins: %s' % ws.blocksize()

Output:

.. testoutput:: ExMSGDiffractionReductionSimple

    Workspace name: irs21360_diffspec_red
    Number of spectra: 1
    Number of bins: 1935

.. categories::
