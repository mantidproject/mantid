
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Mask specified detectors in an instrument.
This algorithm is just a simplfied version of MaskDetectors and has been deprecated.
Please use :ref:`MaskDetectors <algm-MaskDetectors>` instead.

Usage
-----

**Example - MaskInstrument**

.. testcode:: MaskInstrumentExample

  ws = CreateSampleWorkspace()
  ws = MaskInstrument(InputWorkspace=ws, DetectorIDs='100,102-104')
  # Detectors are masked but data and mask data is zeroed
  for i in range(6):
    print("Detector {} masked: {:5} data {}".format(i, str(ws.getDetector(i).isMasked()), ws.readY(i)[0]))

Output:

.. testoutput:: MaskInstrumentExample

  Detector 0 masked: True  data 0.0
  Detector 1 masked: False data 0.3
  Detector 2 masked: True  data 0.0
  Detector 3 masked: True  data 0.0
  Detector 4 masked: True  data 0.0
  Detector 5 masked: False data 0.3

.. categories::

.. sourcelink::
