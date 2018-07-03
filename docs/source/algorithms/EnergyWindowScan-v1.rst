.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs a data reduction from :ref:`ISISIndirectEnergyTransfer <algm-ISISIndirectEnergyTransfer>`,
before performing the :ref:`ElasticWindowMultiple <algm-ElasticWindowMultiple>` algorithm over the elastic
and inelastic regions of the reduced data to calculate the elastic incoherent scattering factor. There
is also an option to perform the :ref:`MSDFit <algm-MSDFit>` algorithm.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS Energy Window Scan**

.. code-block:: python

    EnergyWindowScan(InputFiles="IRS21360.RAW", Instrument='IRIS', Analyser='graphite',
                             Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                             InelasticRange='0, 0.5', GroupingMethod='Individual', MSDFit=True)

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
