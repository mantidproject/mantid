.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Uses the specular reflection condition along with a supplied theta value
to vertically shift the detectors into a corrected location.

ThetaIn == ThetaOut

and

:math:`2*ThetaOut = tan^{-1}\frac{UpOffset}{BeamOffset}`

For LineDetectors and MultiDetectors, the algorithm uses an average of
grouped detector locations to determine the detector position.

.. categories::


.. testcode:: SpecularReflectionPositionCorrectExample

   # Set up an instrument so that the sample is 1.0 distance from the base of a point detector
   import os
   instrument_def = os.path.join( config.getInstrumentDirectory() , "INTER_Definition.xml")
   ws = LoadEmptyInstrument(instrument_def)
   inst = ws.getInstrument()
   ref_frame = inst.getReferenceFrame()
   vertical_position = {ref_frame.pointingUpAxis(): 0, ref_frame.pointingAlongBeamAxis(): 1.0, ref_frame.pointingHorizontalAxis():0} 
   MoveInstrumentComponent(ws, 'point-detector',RelativePosition=False, **vertical_position)
   MoveInstrumentComponent(ws, 'some-surface-holder',RelativePosition=False,  X=0, Y= 0, Z=0)

   # Correct the detector position. Has a 45 degree incident beam angle.
   corrected_ws = SpecularReflectionPositionCorrect(InputWorkspace=ws, DetectorComponentName='point-detector', AnalysisMode='PointDetectorAnalysis', TwoThetaIn=45.0)

   # Get the detector position post correction. We expect that the vertical offset of the point detector == 1.0
   inst = corrected_ws.getInstrument()
   det_pos = inst.getComponentByName('point-detector').getPos()
   print det_pos


.. testcleanup:: SpecularReflectionPositionCorrectExample
   DeleteWorkspace('ws')
   DeleteWorkspace('corrected_ws')
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectExample 
 
   [0,1,1]
