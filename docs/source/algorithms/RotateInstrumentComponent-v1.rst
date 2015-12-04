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
The centre of rotation is the centre of the component to be rotated,
and any children of that component (such as detectors in a bank, or pixels in a tube) 
are moved and rotated along with the component.

Usage
-----

Example 1: Rotating a bank around the Y Axis
############################################
  
.. figure:: ../images/RotateBank90.png
   :alt: RotateBank90.png‎
   :align: center
   :figwidth: image

   Rotating a bank around the 90° round Y Axis.  The centre of rotation is the centre of the component to be rotated, and any children of that component (such as each detector in this instance) are moved and rotated along with the bank.

.. testcode:: ExBank

  # Load a MUSR file
  musr = Load('MUSR00015189')
  # and use the first workspace in the workspace group
  ws = mtd['musr_1']

  print 'Original positions of detectors 1 and 2'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Det 2',ws.getInstrument().getDetector(2).getPos()

  # Rotate bank 'back' around the Z axis by 90
  RotateInstrumentComponent( ws, ComponentName='back', X=0,Y=1,Z=0, Angle=90.0 )

  print 'Positions of detectors 1 and 2 after rotation'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Det 2',ws.getInstrument().getDetector(2).getPos()


Output
^^^^^^

.. testoutput:: ExBank

  Original positions of detectors 1 and 2
  Det 1 [-0.0888151,-0.108221,0.145]
  Det 2 [-0.0659955,-0.123469,0.145]
  Positions of detectors 1 and 2 after rotation
  Det 1 [-1.38778e-17,-0.108221,0.233815]
  Det 2 [0,-0.123469,0.210996]

Example 2: Rotating a bank around the Z Axis
############################################

.. figure:: ../images/RotateBank3Dets.png
   :alt: RotateBank3Dets.png‎
   :align: center
   :figwidth: image

   Rotating the bank around the Z Axis.  The centre of rotation is the centre of the bank, so the detectors are translated and rotated to match.

.. testcode:: ExBank2

  # Load a MUSR file
  musr = Load('MUSR00015189')
  # and use the first workspace in the workspace group
  ws = mtd['musr_1']

  print 'Original positions of detectors 1 and 4'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Det 2',ws.getInstrument().getDetector(4).getPos()

  # Rotate bank 'back' around the Z axis by 3 detectors.
  RotateInstrumentComponent( ws, ComponentName='back', X=0,Y=0,Z=1, Angle=3*360.0 / 32 )

  print 'Positions of detector 1 after rotation'
  print 'Det 1',ws.getInstrument().getDetector(1).getPos()
  print 'Detector 1 took place of detector 4'

Output
^^^^^^

.. testoutput:: ExBank2

  Original positions of detectors 1 and 4
  Det 1 [-0.0888151,-0.108221,0.145]
  Det 2 [-0.0137224,-0.139326,0.145]
  Positions of detector 1 after rotation
  Det 1 [-0.0137224,-0.139326,0.145]
  Detector 1 took place of detector 4

Example 3: Rotating a single detector
#####################################

.. figure:: ../images/RotateDetector.png
   :alt: RotateDetector.png‎
   :align: center
   :figwidth: image

   Rotating the detector around the Z Axis by 90 degrees.  The centre of rotation is the centre of the detector.

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

.. sourcelink::
