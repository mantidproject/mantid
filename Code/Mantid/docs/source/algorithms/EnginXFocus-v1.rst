.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

Performs a TOF to dSpacing conversion using calibrated pixel positions, focuses the values in dSpacing
and then converts them back to TOF.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Simple focussing of and EnginX data file:**

.. testcode:: ExSimple

   # Run the algorithm
   ws = EnginXFocus(Filename="ENGINX00213855.nxs",
   			   		Bank=1)

   # Should have one spectrum only
   print "No. of spectra:", ws.getNumberHistograms()

   # Print a few arbitrary bins where higher intensities are expected
   fmt = "For TOF of {0:.3f} intensity is {1:.3f}"
   for bin in [3169, 6037, 7124]:
     print fmt.format(ws.readX(0)[bin], ws.readY(0)[bin])

Output:

.. testoutput:: ExSimple

   No. of spectra: 1
   For TOF of 20165.642 intensity is 7.436
   For TOF of 33547.826 intensity is 9.388
   For TOF of 38619.804 intensity is 17.397
   
.. categories::
