.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.


Allows to correct for tiny variations in pixel parameters. It does this by fitting the peak for each of the the selected bank's indices, (using :ref:`algm-EnginXFitPeaks` ) and using the resulting difc value to calibrate the detector position. The calibrated detector position is produced as shown below: 



.. math:: L2 = \left(\frac{Difc} { 252.816 * 2 * sin \left(\frac{2\theta} {2.0}\right)}\right) - 50



The cartesian2d vector is then returned for the given spherical co ordinates (L2, 2theta and phi).

The result of the calibration is accepted by both :ref:`algm-EnginXCalibrate` and 
:ref:`algm-EnginXFocus` to correct the detector positions before focussing.

Expects the *long* calibration run, which provides a decent pattern for every pixel.

.. categories::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculate Difc and Zero:**

.. testcode:: Ex

   table = EnginXCalibrateFull(Filename="ENGINX00213855focussed.nxs",
                                     ExpectedPeaks=[1.097, 2.1], Bank=1)

   pos =  table.column(1)[0]
   print "Det ID:", table.column(0)[0]
   print "Calibrated position: (%.3f,%.3f,%.3f)" % (pos.getX(),pos.getY(),pos.getZ())

Output:

.. testoutput:: Ex

   Det ID: 100001
   Calibrated position: (1.506,0.000,0.002)
