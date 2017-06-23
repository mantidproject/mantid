
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is the loader for the raw `.nxs` files produced by the diffractometers at ILL.
Currently it supports the instruments D20 and D2B.

It is able to treat the data files for arbitrary types of scans.
The output is a point-data workspace with each spectrum corresponding to each of the detector pixels.
The x-axis is the scanned variable axis, if there is a scan, otherwise it is empty (only one bin).

The exception to this is where the scanned variable is the detector, that is for detector scans on D2B.
In this case there is only one bin, and a workspace is produced with time indexed detector information.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - LoadILLDiffraction - D20 no scan**

.. testcode:: LoadILLDiffractionExample

   ws = LoadILLDiffraction(Filename='ILL/D20/967100.nxs')

   print('The output has {0} bins (no scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionExample

   The output has 1 bins (no scan) and 3073 spectra

**Example - LoadILLDiffraction - D20 temperature scan**

.. testcode:: LoadILLDiffractionTemperatureScanExample

   ws = LoadILLDiffraction(Filename='ILL/D20/000017.nxs')

   print('The output has {0} bins (scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionTemperatureScanExample

   The output has 21 bins (scan) and 3073 spectra

**Example - LoadILLDiffraction - D2B detector scan**

.. testcode:: LoadILLDiffractionDetectorScanExample

   ws = LoadILLDiffraction(Filename='ILL/D2B/508093.nxs')

   print('The output has {0} bins (detector scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionDetectorScanExample

   The output has 1 bins (detector scan) and 409625 spectra

.. categories::

.. sourcelink::

