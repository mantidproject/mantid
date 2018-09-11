.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses the Specular reflection condition :math:`\theta_{In} \equiv \theta_{Out}`
to calculate and return a corrected :math:`\theta_{In}`.

.. math:: 

   2\theta = \arctan\left(\frac{UpOffset}{BeamOffset}\right)

The calculated :math:`2\theta` value in degrees is returned by the algorithm.

Also see
:ref:`algm-SpecularReflectionPositionCorrect`

Previous Versions
-----------------

For version 1 of the algorithm, please see
`SpecularReflectionCalculateTheta-v1. <SpecularReflectionCalculateTheta-v1.html>`_. Note
that version 1 worked with detectors at :math:`\theta` rather than
:math:`2\theta` for historical reasons.

Usage
-----

**Example - Point detector theta calculation**

.. testcode:: SpecularReflectionCalculateThetaPointDetectorExample

   # Set up an instrument with a 45 degree final two theta angle.
   import os
   instrument_def = os.path.join( config.getInstrumentDirectory() , "INTER_Definition.xml")
   ws = LoadEmptyInstrument(instrument_def)
   inst = ws.getInstrument()
   ref_frame = inst.getReferenceFrame()
   upoffset = ref_frame.vecPointingUp() 
   det_position = {ref_frame.pointingUpAxis(): 1.0, ref_frame.pointingAlongBeamAxis(): 1.0, ref_frame.pointingHorizontalAxis():0}
   MoveInstrumentComponent(ws, 'point-detector',RelativePosition=False, **det_position)
   MoveInstrumentComponent(ws, 'some-surface-holder',RelativePosition=False,  X=0, Y= 0, Z=0)

   # Calculate the two theta.
   two_theta = SpecularReflectionCalculateTheta(InputWorkspace=ws, DetectorComponentName='point-detector', AnalysisMode='PointDetectorAnalysis')
   print(two_theta)
   
Output:

.. testoutput:: SpecularReflectionCalculateThetaPointDetectorExample 
 
   45.0
  
.. categories::

.. sourcelink::
