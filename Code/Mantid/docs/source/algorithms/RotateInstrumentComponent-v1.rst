.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

RotateInstrumentComponent rotates a component around an axis of rotation
by an angle given in degrees. Rotation by 0 degrees does not change the
component's orientation. The rotation axis (X,Y,Z) must be given in the
co-ordinate system attached to the component and rotates with it.

Usage
-----

Example 1: rotating a bank
##########################

.. testcode:: ExBank

  # Load a MUSR file
  musr = Load('MUSR00015189')
  # and use the first workspace in the workspace group
  ws = mtd['musr_1']

  print 'Original positions of detectors 1 and 2'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Det 2',ws.getInstrument().getDetector(2).getPos()

  # Rotate bank 'back' around the Z axis by 360 / 32 degrees.
  RotateInstrumentComponent( ws, ComponentName='back', X=0,Y=0,Z=1, Angle=360.0 / 32 )

  print 'Positions of detectors 1 and 2 after rotation'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Det 2',ws.getInstrument().getDetector(2).getPos()
  print 'Detector 1 took place of detector 2'

Output
^^^^^^

.. testoutput:: ExBank

  Original positions of detectors 1 and 2
  Det 1 [-0.0888151,-0.108221,0.145]
  Det 2 [-0.0659955,-0.123469,0.145]
  Positions of detectors 1 and 2 after rotation
  Det 1 [-0.0659955,-0.123469,0.145]
  Det 2 [-0.0406399,-0.133972,0.145]
  Detector 1 took place of detector 2

Example 2: rotating a detector
##############################

.. testcode:: ExDet

  import numpy as np

  # Load a MUSR file
  musr = Load('MUSR00015189')
  # and use the first workspace in the workspace group
  ws = mtd['musr_1']

  # Rotating a detector doesn't change its position, just its orientation

  # Original position of detector 33
  print ws.getInstrument().getDetector(33).getPos()

  # Caclulate the solid angles for all detectors in the instrument
  # The result is a single-bin workspace with solid angles for all spectra in ws
  saws = SolidAngle( ws )
  # Collect the solid angles from the first bin in saws and save them in numpy array.
  # Numpy module makes it easy to manipulate arrays
  sa1 = np.array( [ saws.readY(i)[0] for i in range(saws.getNumberHistograms()) ] )

  # Rotate detector 33 around the Z axis by 90 degrees.
  RotateInstrumentComponent( ws, DetectorID=33, X=0,Y=0,Z=1, Angle=90 )

  # Check the position of detector 33 stays unchanged
  print ws.getInstrument().getDetector(33).getPos()

  # Calculate the solid angles after rotation
  saws = SolidAngle( ws )
  sa2 = np.array( [ saws.readY(i)[0] for i in range(saws.getNumberHistograms()) ] )

  # Take element by element difference of the solid angles
  diff = sa2 - sa1
  print diff
  print 'The non-zero difference',diff[32] ,'is due to detector', ws.getDetector(32).getID()

Output
^^^^^^

.. testoutput:: ExDet

  [0.0888151,-0.108221,-0.145]
  [0.0888151,-0.108221,-0.145]
  [ 0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.         -0.04645313  0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.          0.          0.          0.          0.          0.          0.
    0.        ]
  The non-zero difference -0.0464531276188 is due to detector 33

.. categories::
