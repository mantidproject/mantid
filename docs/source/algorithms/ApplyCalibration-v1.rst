.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Update detector calibration from input table workspace. Current calibration properties are:
- Table column *Detector Position*  for absolute positions
- Table column *Detector Y Coordinate* for absolute position along the vertical (Y) axis
- Table column *Detector Height* for detector size along the vertical (Y) axis
- Table column *Detector Width* for detector size along the horizontal (X) axis

The CalibrationTable must have the column *Detector ID* (integer) and one or more of the following column(s) options:
- column *Detector Position* of type :py:obj:`V3Ds <mantid.kernel.V3D>`
- columns *Detector Y Coordinate* (float) and *Detector Height* (float)
- column *Detector Width* (float)

This algorithm is not appropriate for rectangular detectors and won't move them.

Usage
-----
**Example - move three detectors to specified positions that would be got from calibration**

.. include:: ../usagedata-note.txt 

.. testcode:: ExApplyCalibSimple

   from mantid.kernel import V3D
   # Create Workspace with instrument
   # We use HRP workspace, which has detectors that are not rectangular.
   ws = Load("HRP39180.RAW")
   compInfo = ws.componentInfo()
   detInfo = ws.detectorInfo()

   spectra = [0, 3] # Spectra of detectors to be moved

   # Show positions before calibration
   for i in spectra:
       det = ws.getDetector(i)
       xyz = det.getPos()
       print("Position of Detector ID=%i before ApplyCalibration: %.1f,%.1f,%.1f" % (det.getID(), xyz.X(), xyz.Y(), xyz.Z()))
       index = detInfo.indexOf(det.getID())  # detectorInfo and componentInfo index
       box = compInfo.shape(index).getBoundingBox().width()
       scalings = compInfo.scaleFactor(index)
       width, height = box[0] * scalings[0], box[1] * scalings[1]
       print('Width and Height of Detector ID=%i before ApplyCalibration: %.3f %.3f' % (det.getID(), width, height))

   # Create CalibrationTable - This would be done by the calibration functions
   calibTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
   # Add required columns
   calibTable.addColumn(type="int",name="Detector ID")
   calibTable.addColumn(type="V3D",name="Detector Position")
   calibTable.addColumn(type="double",name="Detector Y Coordinate")
   calibTable.addColumn(type="double",name="Detector Height")
   calibTable.addColumn(type="double",name="Detector Width")
   # Populate the columns for three detectors
   detIDList = [ws.getDetector(spectra[0]).getID(), ws.getDetector(spectra[1]).getID() ]
   detPosList = [V3D(9.0,0.0,0.0), V3D(10.0,3.0,0.0)]
   detYCoord = [0.1, 2.9]
   detWidthList = [0.011, 0.009]
   detHeightList = [0.043, 0.052]
   for j in range(len(detIDList)):
       nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j], 'Detector Y Coordinate': detYCoord[j],
                           'Detector Width': detWidthList[j], 'Detector Height': detHeightList[j]}
       calibTable.addRow ( nextRow )

   # Apply Calibration to Workspace
   ApplyCalibration(Workspace=ws, CalibrationTable=calibTable)

   # Show positions after calibration
   for i in spectra:
       det = ws.getDetector(i)
       xyz = det.getPos()
       print("Position of Detector ID=%i after ApplyCalibration: %.1f,%.1f,%.1f" % (det.getID(), xyz.X(), xyz.Y(), xyz.Z()))
       index = detInfo.indexOf(det.getID())  # detectorInfo and componentInfo index
       box = compInfo.shape(index).getBoundingBox().width()
       scalings = compInfo.scaleFactor(index)
       width, height = box[0] * scalings[0], box[1] * scalings[1]
       print('Width and Height of Detector ID=%i after ApplyCalibration: %.3f %.3f' % (det.getID(), width, height))

Output:

.. testoutput:: ExApplyCalibSimple

   Position of Detector ID=1100 before ApplyCalibration: 0.0,0.1,-0.9
   Width and Height of Detector ID=1100 before ApplyCalibration: 0.010 0.049
   Position of Detector ID=1103 before ApplyCalibration: 0.0,0.1,-0.9
   Width and Height of Detector ID=1103 before ApplyCalibration: 0.010 0.054
   Position of Detector ID=1100 after ApplyCalibration: 9.0,0.1,0.0
   Width and Height of Detector ID=1100 after ApplyCalibration: 0.011 0.043
   Position of Detector ID=1103 after ApplyCalibration: 10.0,2.9,0.0
   Width and Height of Detector ID=1103 after ApplyCalibration: 0.009 0.052

.. categories::

.. sourcelink::
