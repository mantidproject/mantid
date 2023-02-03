
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the loader for the raw `.nxs` files produced by the powder and liquid diffractometers at ILL.
Currently it supports the instruments D20, D1B, D2B, and D4.

The *TwoThetaOffset* parameter corresponding to the rotation of the detector is optional and only used for D1B. It is to be provided in degrees.

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

The loader creates time series logs for each of the scanned variable in the `.nxs` file. The scanned variable name is added under `ScanVar` log.

D2B alignment
-------------

For D2B, the loader applies tube alignment; first rotating them horizontally around the sample, then translating them vertically.
This is done based on *tube_centers* and *tube_angles* parameters defined in the Instrument Parameter File.

D2B pixel numbering in tubes
----------------------------

For D2B it assumes that the counts written in the nexus file follow "U-shape" convention; that is, the first tube counts are written from bottom to top, the second one: from top to bottom, and so on.
Note that in the IDF of D2B detector IDs grow ascending from bottom to top for all the tubes.
The loader takes care that the correct counts are attributed to correct pixels.

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

**Example - LoadILLDiffraction - D1B**

.. testcode:: LoadILLDiffractionD1BExample

  ws = LoadILLDiffraction(Filename='ILL/D1B/473432.nxs')

  print('The output has {0} bin (detector scan) and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionD1BExample

  The output has 1 bin (detector scan) and 1281 spectra

**Example - LoadILLDiffraction - D4**

.. testcode:: LoadILLDiffractionD4Example

   ws = LoadILLDiffraction(Filename='ILL/D4/387230.nxs')

   print('The output has {0} bin and {1} spectra'.format(ws.blocksize(), ws.getNumberHistograms()))

Output:

.. testoutput:: LoadILLDiffractionD4Example

   The output has 1 bin and 577 spectra

.. categories::

.. sourcelink::
