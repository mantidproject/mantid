.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Update detector positions from input table workspace. The positions are
updated as absolute positions and so this update can be repeated.

The PositionTable must have columns *Detector ID* and *Detector
Position*. The entries of the *Detector ID* column are integer referring
to the Detector ID and the enties of the *Detector Position* are
`V3Ds <../api/python/mantid/kernel/V3D.html>`__ referring to the position of the detector whose ID is in same row.

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

   spectra = [0, 1, 3] # Spectra of detectors to be moved

   # Show positions before calibration
   for i in spectra:
        det = ws.getDetector(i)
        print("Position of Detector ID=%i before ApplyCalibration: %.0f,%.0f,%.0f" % (det.getID(), 
                det.getPos().X(), det.getPos().Y(), det.getPos().Z()))


   # Create PositionTable - This would be done by the calibration functions
   calibTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
   # Add required columns
   calibTable.addColumn(type="int",name="Detector ID")  
   calibTable.addColumn(type="V3D",name="Detector Position")
   # Populate the columns for three detectors
   detIDList = [ ws.getDetector(spectra[0]).getID(), ws.getDetector(spectra[1]).getID(), ws.getDetector(spectra[2]).getID() ]
   detPosList = [ V3D(9.0,0.0,0.0), V3D(10.0,3.0,0.0), V3D(12.0,3.0,6.0)]
   for j in range(len(detIDList)):
        nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j] }
        calibTable.addRow ( nextRow )


   # Apply Calibration to Workspace
   ApplyCalibration ( Workspace=ws, PositionTable=calibTable)

   # Show positions after calibration
   for i in spectra:
        det = ws.getDetector(i)
        print("Position of Detector ID=%i after ApplyCalibration: %.0f,%.0f,%.0f" % (det.getID(), 
                det.getPos().X(), det.getPos().Y(), det.getPos().Z()))

Output:

.. testoutput:: ExApplyCalibSimple

   Position of Detector ID=1100 before ApplyCalibration: 0,0,-1
   Position of Detector ID=1101 before ApplyCalibration: 0,0,-1
   Position of Detector ID=1103 before ApplyCalibration: 0,0,-1
   Position of Detector ID=1100 after ApplyCalibration: 9,0,0
   Position of Detector ID=1101 after ApplyCalibration: 10,3,0
   Position of Detector ID=1103 after ApplyCalibration: 12,3,6

.. categories::

.. sourcelink::
