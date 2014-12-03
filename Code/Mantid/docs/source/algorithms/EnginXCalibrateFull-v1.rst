.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.

Fits the data of every detector pixel using expected Bragg peak positions.

Allows to correct for tiny variations in pixel parameters, through using :ref:`algm-EnginXCalibrate` for every single pixel position.

The result of the calibration is accepted by both :ref:`algm-EnginXCalibrate` and 
:ref:`algm-EnginXFocus` to correct the detector positions before focussing.

Expects the *long* calibration run, which provides a decent pattern for every pixel.

After receiving the bank's indicies, it goes through each one fitting the peak (using :ref:`algm-EnginXFitPeak`) and using the resulting difc value to calibrate the detector position. The calibrated detector position is produced as shown below: 

#test12



.. math:: L2 = \left(\frac{Difc} { 252.816 * 2 * sin \left(\frac{det2\theta} {2.0}\right)}\right) - 50



the cartesian2d vector is then returned for the given spherical co ordinates

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
