
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Mask specified detectors in an instrument.
This is does *not* clear the data in associated spectra in the workspace.
To clear the data manually ``ClearMaskedSpectra`` can be called.

Usage
-----

**Example - MaskInstrument**

.. testcode:: MaskInstrumentExample

  ws = CreateSampleWorkspace()
  masked = MaskInstrument(InputWorkspace=ws, DetectorIDs='100,102-104')
  # Detectors are masked but data is untouched
  for i in range(6):
    print("Detector {} masked: {} data {}".format(i, masked.getDetector(i).isMasked(), masked.readY(i)[0]))

Output:

.. testoutput:: MaskInstrumentExample

  Detector 0 masked: True data 0.3
  Detector 1 masked: False data 0.3
  Detector 2 masked: True data 0.3
  Detector 3 masked: True data 0.3
  Detector 4 masked: True data 0.3
  Detector 5 masked: False data 0.3

.. categories::

.. sourcelink::

