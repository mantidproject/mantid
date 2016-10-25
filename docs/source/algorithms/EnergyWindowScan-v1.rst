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


Usage
-----

.. include:: ../usagedata-note.txt

**Example - IRIS Energy Window Scan**

.. testcode:: ExDataFit

    EnergyWindowScan(InputFiles="IRS26176.RAW", Instrument='IRIS', Analyser='graphite',
                             Reflection='002', SpectraRange='3, 50', ElasticRange='-0.5, 0',
                             InelasticRange='0, 0.5', GroupingMethod='Individual', MSDFit=True)

    print("MSD Result:")
    print 'A0: ' + str(mtd['Scan_msd'].readY(0))
    print 'A1: ' + str(mtd['Scan_msd'].readY(1))

Output:

.. testoutput:: ExGeneratedDataFit

    MSD Result:
    A0: [-2.45452609]
    A1: [ 0.16692898]

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
