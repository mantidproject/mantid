.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an empty workspace containing the VESUVIO instrument using
:ref:`LoadEmptyInstrument <algm-LoadEmptyInstrument>` and updates the detector
positions using a PAR file using the :ref:`UpdateInstrumentFromFile
<algm-UpdateInstrumentFromFile>` algorithm.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - LoadDefaultDetectorPositions**

.. testcode:: exLoadDefaultDetectorPositions

    evs_ws = LoadEmptyVesuvio()

    evs = evs_ws.getInstrument()

    sample = evs.getSample()
    b_det_1 = evs_ws.getDetector(2)

    b_det_1_l1 = sample.getPos().distance(b_det_1.getPos())

    print("First backscattering detector L1 = {:.5f}m".format(b_det_1_l1))

Output:

.. testoutput:: exLoadDefaultDetectorPositions

    First backscattering detector L1 = 0.67477m

**Example - LoadWithPARFileDetectorPositions**

.. testcode:: exLoadWithPARFileDetectorPositions

    evs_ws = LoadEmptyVesuvio(InstrumentParFile='IP0005.dat')

    evs = evs_ws.getInstrument()

    sample = evs.getSample()
    b_det_1 = evs_ws.getDetector(2)

    b_det_1_l1 = sample.getPos().distance(b_det_1.getPos())

    print("First backscattering detector L1 = {:.5f}m".format(b_det_1_l1))

Output:

.. testoutput:: exLoadWithPARFileDetectorPositions

    First backscattering detector L1 = 0.67080m

.. categories::

.. sourcelink::
