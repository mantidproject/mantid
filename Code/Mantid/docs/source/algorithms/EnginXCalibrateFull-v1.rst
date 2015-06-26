.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument. It might get changed or even 
   removed without a notification, should instrument scientists decide to do so.


Allows to calibrate or correct for variations in detector position
parameters. It does this by fitting the peaks for each of the selected
bank's detector indices, (using :ref:`algm-EnginXFitPeaks` as a child
algorithm) and using the resulting difc values to calibrate the
detector positions.

This algorithm produces a table with calibration information that can
be inspected and used in other algorithms. The algorithm also applies
that calibration on the input workspace. After running this algorithm
the resulting calibration can be checked by inspecting the output
table values and/or visualizing the input workspace instrument in the
instrument viewer.

This algorithm produces a table with calibration information
(calibrated or corrected positions and parameters). The calibrated
detector positions are calculated as follows:

.. math:: L2 = \left(\frac{Difc} { 252.816 * 2 * sin \left(\frac{2\theta} {2.0}\right)}\right) - 50

The output table gives, for every detector, the original position
before calibration (as a V3D point, x-y-z values), the new calibrated
position (as V3D), the calibrated spherical co-ordinates (L2, :math:`2
\theta`, :math:`\phi`). It also gives the variation in the L2
position, and the 'difc' and 'zero' calibration parameters.

The result of the calibration (the DetectorPositions output table) is
accepted by both :ref:`algm-EnginXCalibrate` and
:ref:`algm-EnginXFocus` which use the columns 'Detector ID' and
'Detector Position' of the table to correct the detector positions
before focussing.

Expects the *long* calibration run, which provides a decent pattern
for every pixel.

.. categories::

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculate corrected positions, and difc and zero:**

.. testcode:: ExCalFull

   ws_name = 'ws_focussed'
   Load('ENGINX00213855focussed.nxs', OutputWorkspace=ws_name)

   posTable = EnginXCalibrateFull(Workspace=ws_name,
                                  ExpectedPeaks=[1.097, 2.1], Bank=1)

   detID = posTable.column(0)[0]
   calPos =  posTable.column(2)[0]
   print "Det ID:", detID
   print "Calibrated position: (%.3f,%.3f,%.3f)" % (calPos.getX(), calPos.getY(), calPos.getZ())
   ws = mtd[ws_name]
   posInWSInst = ws.getInstrument().getDetector(detID).getPos()
   print "Is the detector position calibrated now in the original workspace instrument?", (calPos == posInWSInst)

.. testcleanup:: ExCalFull

   DeleteWorkspace(ws_name)

Output:

.. testoutput:: ExCalFull

   Det ID: 100001
   Calibrated position: (1.506,0.000,0.002)
   Is the detector position calibrated now in the original workspace instrument? True

**Example - Calculate corrected positions, saving in a file:**

.. testcode:: ExCalFullWithOutputFile

   import os, csv

   ws_name = 'ws_focussed'
   pos_filename = 'detectors_pos.csv'
   Load('ENGINX00213855focussed.nxs', OutputWorkspace=ws_name)
   posTable = EnginXCalibrateFull(Workspace=ws_name,
                                  ExpectedPeaks=[1.097, 2.1], Bank=1,
                                  OutputDetectorPositionsFilename=pos_filename)

   detID = posTable.column(0)[0]
   pos =  posTable.column(2)[0]
   print "Det ID:", detID
   print "Calibrated position: (%.3f,%.3f,%.3f)" % (pos.getX(),pos.getY(),pos.getZ())
   print "Was the file created?", os.path.exists(pos_filename)
   with open(pos_filename) as csvf:
      reader = csv.reader(csvf, dialect='excel')
      reader.next()
      calibOK = True
      for i,row in enumerate(reader):
         calPos = posTable.column(2)[i]
         calibOK = calibOK and (abs(float(row[4]) - calPos.getX()) < 1e-6) and\
                   (abs(float(row[5]) - calPos.getY()) < 1e-6) and\
                   (abs(float(row[6]) - calPos.getZ()) < 1e-6)
         if not calibOK: break
   print "Does the calibration file have the expected values?", calibOK

.. testcleanup:: ExCalFullWithOutputFile

   DeleteWorkspace(ws_name)
   import os
   os.remove(pos_filename)

Output:

.. testoutput:: ExCalFullWithOutputFile

   Det ID: 100001
   Calibrated position: (1.506,0.000,0.002)
   Was the file created? True
   Does the calibration file have the expected values? True
