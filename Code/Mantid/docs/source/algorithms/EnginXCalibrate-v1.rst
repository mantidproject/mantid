.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

The calibration run is focussed and then fitted using expected Bragg peak positions.

Allows to correct for deviations in sample position.

.. categories::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculate Difc and Zero:**

.. testcode:: Ex

   Difc, Zero = EnginXCalibrate(Filename="ENGINX00213855.nxs",
                                     ExpectedPeaks=[1.097, 2.1], Bank=1)

   print "Difc: %.2f" % (Difc)
   print "Zero: %.2f" % (Zero)

Output:

.. testoutput:: Ex

   Difc: 18404.93
   Zero: -10.89
