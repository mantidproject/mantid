.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Runs the :ref:`EnergyWindowScan <algm-EnergyWindowScan>` algorithm with slightly customised inputs and
provides options for plotting and saving output.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS Energy Window Scan**

.. code-block:: python

    IndirectQuickRun(RunNumbers="21360", Instrument='IRIS', Analyser='graphite',
                     Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                     InelasticRange='0, 0.5', GroupingMethod='Individual', MSDFit=True)

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
