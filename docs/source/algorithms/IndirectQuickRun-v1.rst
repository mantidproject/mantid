.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Runs the :ref:`EnergyWindowScan <algm-EnergyWindowScan>` algorithm with slightly customised inputs. It also
provides the option to perform a Width Fit which utilizes the :ref:`IndirectTwoPeakFit <algm-IndirectTwoPeakFit>`
algorithm.

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
