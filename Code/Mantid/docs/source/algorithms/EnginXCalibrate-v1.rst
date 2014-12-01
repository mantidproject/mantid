.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.


Utilises :ref:`algm-EnginXFitPeaks` which determines peaks through peak function and provides the Difc and ZERO params to the Calibrate algorithm. It then calls :ref:`algm-EnginXFocus` which performs a TOF to dSpacing conversion using calibrated pixel positions, focuses the values in dSpacing and then converts them back to TOF.
For this algorithm it is the indirect calibration of the sample position that is calibrated and the result of this is returned as Difc and Zero rather than an actual new sample position (hence the reason for 'indirect').
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
