.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Iterates over the input workspace evaluating the test for the first bin in each spectrum. There are two output modes:

Output to a calfile
   For this mode the properties ``InputCalFile`` and ``OutputCalFile`` have to be set.
   If the detectors should be masked it deselects all of
   the contributing detectors in the output calfile. All other aspects of
   the ``InputCalFile`` are copied over to the ``OutputCalFile``.

Output to a workspace
   For this mode the ``OutputWorkspace`` property has to be set. The algorithm masks the selected detectors for the output workspace. This does not clear the data in the spectra under question. Use :ref:`ClearMaskedSpectra <algm-ClearMaskedSpectra>` to clear the masked data manually.

Usage
-----

**Example - mask spectra whose integral is below some limit**

.. testcode:: MaskDetectorsIfEx

   ws = CreateSampleWorkspace()
   # One spectrum has lower counts than the rest
   badIndex = 3
   Ys = ws.dataY(badIndex)
   Ys *= 0.01
   integral = Integration(ws)
   MaskDetectorsIf(integral, Operator='Less', Value=10., OutputWorkspace=integral)
   MaskDetectors(ws, MaskedWorkspace=integral)
   # Inspect the result
   isMasked = ws.spectrumInfo().isMasked(badIndex)
   print('Spectrum at index {} is masked? {}'.format(badIndex, isMasked))
   
Output:

.. testoutput:: MaskDetectorsIfEx

   Spectrum at index 3 is masked? True

.. categories::

.. sourcelink::
