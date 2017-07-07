
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is the loader for the raw `.nxs` files produced by the diffractometers at ILL.
Currently it supports the instruments D20 and D1B.
It is able to treat the data files for arbitrary types of scans.
Output is a point-data workspace with each spectrum corresponding to each of the detector pixel.
X-axis is the scanned variable axis, if there is a scan, otherwise it is empty (only one bin).

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - LoadILLDiffraction (no scan)**

.. testcode:: LoadILLDiffractionExample

   ws = LoadILLDiffraction(Filename='ILL/D20/967100.nxs')

   print('The output has {0} bins (no scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionExample

   The output has 1 bins (no scan) and 3073 spectra

**Example - LoadILLDiffraction (scan)**

.. testcode:: LoadILLDiffractionScanExample

   ws = LoadILLDiffraction(Filename='ILL/D20/000017.nxs')

   print('The output has {0} bins (scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionScanExample

   The output has 21 bins (scan) and 3073 spectra

.. categories::

.. sourcelink::

