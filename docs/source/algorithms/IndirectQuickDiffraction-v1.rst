.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs a data reduction from :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`,
before performing the :ref:`ElasticWindowMultiple <algm-ElasticWindowMultiple>` algorithm over the elastic
and inelastic regions of the reduced data to calculate the elastic incoherent scattering factor. There
is also an option to perform the :ref:`MSDFit <algm-MSDFit>` algorithm.

This quick run also runs :ref:`ISISIndirectDiffractionReduction <algm-ISISIndirectDiffractionReduction>` over diffraction spectra.


Usage
-----

.. include:: ../usagedata-note.txt

**Example**

.. testcode:: IndirectQuickDiffraction

    IndirectQuickDiffraction(RunNumbers='IRS21360.RAW',Instrument='IRIS', Analyser='graphite', Reflection='002', SpectraRange=[3,53],
                             ElasticRange=[-0.5,0], InelasticRange=[0,0.5],GroupingMethod='All', DiffractionSpectra=[105,112])


.. categories::

.. sourcelink::
