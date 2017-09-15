.. algorithm::

.. summary::

.. alias::

.. properties::

.. warning::

   This algorithm is deprecated (April-2017). Please, use :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` instead.


Description
-----------

A version of :ref:`ISISIndirectDiffractionReduction
<algm-ISISIndirectDiffractionReduction>` specific to use with VESUVIO (EVS)
data, the reduction is performed in the same way however there is the additional
option to load a PAR file.

Workflow
--------

.. diagram:: VesuvioDiffractionReduction-v1_wkflw.dot


Usage
-----

**Example - Running VesuvioDiffractionReduction.**

.. testcode:: ExVesuvioDiffractionReductionSimple

    VesuvioDiffractionReduction(InputFiles='15289',
                            OutputWorkspace='DiffractionReductions',
                            InstrumentParFile='IP0005.dat')

    ws = mtd['DiffractionReductions'].getItem(0)

    print 'Workspace name: %s' % ws.name()
    print 'Number of spectra: %d' % ws.getNumberHistograms()
    print 'Number of bins: %s' % ws.blocksize()

Output:

.. testoutput:: ExVesuvioDiffractionReductionSimple

    Workspace name: vesuvio15289_diffspec_red
    Number of spectra: 1
    Number of bins: 3875

.. categories::

.. sourcelink::
