
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Clear counts (or events, if applicable) on all spectra that are fully masked.
A spectrum is fully masked if all of its associated detectors are masked, e.g., from a call to `MaskInstrument`.

Usage
-----

**Example - ClearMaskedSpectra**

.. testcode:: ClearMaskedSpectraExample

  ws = CreateSampleWorkspace()
  ws = MaskInstrument(InputWorkspace=ws, DetectorIDs='100,102-104')
  ws = ClearMaskedSpectra(InputWorkspace=ws)
  # Detectors are masked but data is untouched
  for i in range(6):
    print("Detector {} masked: {:5} data {}".format(i, str(ws.getDetector(i).isMasked()), ws.readY(i)[0]))

Output:

.. testoutput:: ClearMaskedSpectraExample

  Detector 0 masked: True  data 0.0
  Detector 1 masked: False data 0.3
  Detector 2 masked: True  data 0.0
  Detector 3 masked: True  data 0.0
  Detector 4 masked: True  data 0.0
  Detector 5 masked: False data 0.3

.. categories::

.. sourcelink::

