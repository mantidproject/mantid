
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the loader for the raw `.nxs` files produced by the powder diffractometers at ILL.
Currently it supports the instruments D20 and D2B.

Loading D20
###########

For D20 1-dimensional detector, it supports 3 resolution modes:

- **low** is the physical configuration, i.e. 1536 pixels. This corresponds to the IDF *D20_lr*.

- **nominal** is the default configuration, i.e. pixels split by 2 by DAQ, so in total 3072 pixels. This corresponds to IDF *D20*.

- **high** is when each pixel is split by 3, resulting in 4608 pixels. This corresponds to IDF *D20_hr*.

Note, that all the IDFs contain only active pixels, and do not count the last 2 banks which are permanently inactive.

The *2theta* value of the first pixel is read from the file, and the whole detector is rotated correspondingly.

Scans
#####

The loader is able to load the following scan configurations:

- **no scan**, used for D20, when a single file contains a single dataset, e.g. data acquired with static detector at a single temperature point. In this case x-axis is just a single point.

- **detector scan**, used always for D2B, and for D20 calibration runs, when the detector moves during the run. In this configuration the output is a *scanning workspace* containing one spectrum for each pixel at each time index. The x-axis is again a single point.

- **other scan**, e.g. omega scan for D20, which is another type of motor scan, but it is not the detector that moves, but the sample. In this case, the data in the raw file is organised just as for *detector scan*, but the output workspace is not a *scanning workspace*. It is a regular workspace with x-axis corresponding to the scanned variable, e.g. omega angle.

Logs
####

The loader creates time series logs for each of the scanned variable in the `.nxs` file.

D2B alignment
-------------

For D2B, the loader applies tube alignment; first rotating them horizontally around the sample, then translating them vertically.
This is done based on *tube_centers* and *tube_angles* parameters defined in the Instrument Parameter File.

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

**Example - LoadILLDiffraction - D20 omega scan**

.. testcode:: LoadILLDiffractionOmegaScanExample

   ws = LoadILLDiffraction(Filename='ILL/D20/000017.nxs')

   print('The output has {0} bins (scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionOmegaScanExample

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
