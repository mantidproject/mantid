.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a data reduction from :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`,
before performing the :ref:`SofQW <algm-SofQW>` algorithm and :ref:`SofQWMoments <algm-SofQWMoments>`.
Width and diffusion workspaces are then created.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Running SofQWMoments from with an SofQW workspace.**

.. testcode:: ExSofQWMomentsScan

    reduced, sqw, moments = SqwMomentsScan(InputFiles='IRS21360', LoadLogFiles=False, Instrument='IRIS', Analyser='graphite', Reflection='002', SpectraRange='3,53',
                                           QRange='0,0.1,2', EnergyRange='-0.4,0.01,0.4')


.. testcleanup:: ExSofQWMomentsScan

   DeleteWorkspace(reduced)
   DeleteWorkspace(sqw)
   DeleteWorkspace(moments)

.. categories::

.. sourcelink::
