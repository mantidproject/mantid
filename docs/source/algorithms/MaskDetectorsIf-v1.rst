.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Iterates over the input workspace evaluating the test for each bin in each spectrum, and will select (or deselect) it is any of the bins match the test. There are two output modes:

Output to a calfile
   For this mode the properties ``InputCalFile`` and ``OutputCalFile`` have to be set.
   If the detectors should be masked it deselects all of
   the contributing detectors in the output calfile. All other aspects of
   the ``InputCalFile`` are copied over to the ``OutputCalFile``.

Output to a workspace
   For this mode the ``OutputWorkspace`` property has to be set. The algorithm masks the selected detectors (:literal:`Mode='SelectIf'`) or unmasks deselected ones (:literal:`Mode='DeselectIf'`) for the output workspace. Masking will clear the data in the spectra under question. Unmasking will just clear the masking flag but will not restore the data.

.. note:: If you select the NotFinite operator, the value property is ignored.  This operator finds values that are infinite or `NaN. <https://en.wikipedia.org/wiki/NaN>`_

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
