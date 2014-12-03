.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.


Utilises  :ref:`algm-EnginXFocus` which performs a TOF to dSpacing conversion using calibrated pixel positions, focuses the values in dSpacing and then converts them back to TOF. 
Then calls  :ref:`algm-EnginXFitPeaks` which through a sequence of peak fits determines a linear relationship between dSpacing and measured TOF values in terms of DIFC and ZERO values and provides the these parameters to the Calibrate algorithm.
This algorithm provides an indirect calibration of the sample position, that is, a calibration returned in terms of Difc and Zero rather than an actual new sample position (hence the reason for 'indirect').


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
