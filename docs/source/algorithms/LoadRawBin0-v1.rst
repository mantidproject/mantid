.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads just bin0 for all spectra from the given ISIS RAW file and stores it
in a 2D workspace.  You may optionally specify certain spectra to read in.

Usage
-----

**Example - Loading in Bin0 Data**

.. include:: ../usagedata-note.txt

.. testcode:: Ex

   #Bin 0 contains any data outside of the time range.
   bin_zeroes = LoadRawBin0("IRS21360.raw")
   total = SumSpectra(bin_zeroes)

   print("Bin0 contained {:.0f} counts.".format(total.readY(0)[0]))

Output:

.. testoutput:: Ex

   Bin0 contained 55 counts.

.. categories::

.. sourcelink::
