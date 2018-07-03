.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Moves the specified detector component so that the angle between the beam and
the sample-to-detector vector is :literal:`TwoTheta`. The detector component is moved as a
block. The rest of the instrument components remain in the original position. The component
can be shifted vertically (default), or rotated around the sample position.

Previous Versions
-----------------

For version 1 of the algorithm, please see `SpecularReflectionPositionCorrect-v1. <SpecularReflectionPositionCorrect-v1.html>`_

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Correct 'point-detector'**

.. testcode:: SpecularReflectionPositionCorrectPointDetector

   print('point-detector')
   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print('Original position: ' + str(instr.getComponentByName('point-detector').getPos()))

   polref_vert = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='point-detector', DetectorCorrectionType='VerticalShift')
   instr = polref_vert.getInstrument()
   print('Vertical shift:    ' + str(instr.getComponentByName('point-detector').getPos()))

   polref_rot = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='point-detector', DetectorCorrectionType='RotateAroundSample')
   instr = polref_rot.getInstrument()
   print('Rotated:           ' + str(instr.getComponentByName('point-detector').getPos()))
   
Output:

Note that in this case the difference between shifting the detectors vertically or rotating them is negligible.

.. testoutput:: SpecularReflectionPositionCorrectPointDetector 

   point-detector
   Original position: [25.6,0,0.0444961]
   Vertical shift:    [25.6,0,0.0444753]
   Rotated:           [25.6,0,0.0444753]

**Example - Correct 'lineardetector'**

.. testcode:: SpecularReflectionPositionCorrectLinearDetector

   print('lineardetector')
   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print('Original position: ' + str(instr.getComponentByName('lineardetector').getPos()))

   polref_vert = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='lineardetector')
   instr = polref_vert.getInstrument()
   print('Vertical shift:    ' + str(instr.getComponentByName('lineardetector').getPos()))

   polref_rot = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='lineardetector', DetectorCorrectionType='RotateAroundSample')
   instr = polref_rot.getInstrument()
   print('Rotated:           ' + str(instr.getComponentByName('lineardetector').getPos()))
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectLinearDetector 

   lineardetector
   Original position: [26,0,0]
   Vertical shift:    [26,0,0.0513177]
   Rotated:           [25.9996,0,0.0513102]

**Example - Correct 'OSMOND'**

.. testcode:: SpecularReflectionPositionCorrectOSMONDDetector

   print('OSMOND')
   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print('Original position: ' + str(instr.getComponentByName('OSMOND').getPos()))

   polref_vert = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='OSMOND')
   instr = polref_vert.getInstrument()
   print('Vertical shift:    ' + str(instr.getComponentByName('OSMOND').getPos()))

   polref_rot = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='OSMOND', DetectorCorrectionType='RotateAroundSample')
   instr = polref_rot.getInstrument()
   print('Rotated:           ' + str(instr.getComponentByName('OSMOND').getPos()))
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectOSMONDDetector 

   OSMOND
   Original position: [26,0,0]
   Vertical shift:    [26,0,0.0513177]
   Rotated:           [25.9996,0,0.0513102]

.. categories::

.. sourcelink::
