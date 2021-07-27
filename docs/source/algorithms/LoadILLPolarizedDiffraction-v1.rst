
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is the loader for the raw `.nxs` files produced by the polarized diffractometers and spectrometers at ILL.
Currently it supports only the D7 instrument. Workspaces in the loaded group are sorted according to their flipper
state value for each present polarisation orientation, with the first workspace having always flipper 'ON',
and the next 'OFF'.

Loading D7
##########

For D7 1-dimensional detector, it supports 3 types of pixel alignment relative to detector banks:

- **None** is the default alignment, where the pixel positions in banks are set as defined in the instrument definition file

- **Nexus** is the alignment option where the pixel positions relative to banks are read from data available in D7 raw `.nxs` files

- **YIGFile** is the alignment option that uses an Intrument Parameter File filled with *2theta* positions for each pixel relative to the bank *2theta*

All alignment methods allow for independent movement of banks.

The *2theta* value of each bank is read from the file, and it refers to the middle point of the bank. The sum of pixel's and bank *2theta* determine their final position in 3D space.


Logs
####

The loader creates logs separately for each entry present in the `.nxs` file.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - LoadILLPolarizedDiffraction - D7 spectroscopy**

.. testcode:: LoadILLPolarizedDiffractionTOFExample

   wsGroup = LoadILLPolarizedDiffraction(Filename='ILL/D7/395850.nxs')
   print('The output has {0} entries'.format(wsGroup.getNumberOfEntries()))
   ws1 = wsGroup.getItem(0)
   print('Each entry has {0} bins (Time of Flight mode) and {1} spectra'.format(ws1.blocksize(), ws1.getNumberHistograms()))

Output:

.. testoutput:: LoadILLPolarizedDiffractionTOFExample

   The output has 2 entries
   Each entry has 512 bins (Time of Flight mode) and 134 spectra

**Example - LoadILLPolarizedDiffraction - D7 NeXuS alignment monochromatic**

.. testcode:: LoadILLPolarizedDiffractionMonoExample

   wsGroup = LoadILLPolarizedDiffraction(Filename='ILL/D7/401800.nxs')
   print('The output has {0} entries'.format(wsGroup.getNumberOfEntries()))
   ws1 = wsGroup.getItem(0)
   print('Each entry has {0} bins (monochromatic mode) and {1} spectra'.format(ws1.blocksize(), ws1.getNumberHistograms()))

Output:

.. testoutput:: LoadILLPolarizedDiffractionMonoExample

   The output has 6 entries
   Each entry has 1 bins (monochromatic mode) and 134 spectra

.. categories::

.. sourcelink::
