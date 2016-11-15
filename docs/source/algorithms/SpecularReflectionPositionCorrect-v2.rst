.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Moves the specified detector component vertically so that the angle between the beam and
the sample-to-detector vector is :literal:`TwoTheta`. The detector component is moved as a
block. The rest of the instrument components remain in the original position.

Previous Versions
-----------------

For version 1 of the algorithm, please see `SpecularReflectionPositionCorrect-v1. <SpecularReflectionPositionCorrect-v1.html>`_

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Correct 'point-detector'**

.. testcode:: SpecularReflectionPositionCorrectPointDetector

   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print instr.getComponentByName('point-detector').getPos()

   polref = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='point-detector')
   instr = polref.getInstrument()
   print instr.getComponentByName('point-detector').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectPointDetector 
 
   [25.6,0,0]
   [25.6,0,0.0444753]

**Example - Correct 'lineardetector'**

.. testcode:: SpecularReflectionPositionCorrectLinearDetector

   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print instr.getComponentByName('lineardetector').getPos()

   polref = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='lineardetector')
   instr = polref.getInstrument()
   print instr.getComponentByName('lineardetector').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectLinearDetector 
 
   [26,0,0]
   [26,0,0.0513177]

**Example - Correct 'OSMOND'**

.. testcode:: SpecularReflectionPositionCorrectOSMONDDetector

   polref = Load(Filename=r'POLREF00004699.raw', PeriodList=1)
   polref = polref[0]

   instr = polref.getInstrument()
   print instr.getComponentByName('OSMOND').getPos()

   polref = SpecularReflectionPositionCorrect(polref, TwoTheta = 2*0.49, DetectorComponentName='OSMOND')
   instr = polref.getInstrument()
   print instr.getComponentByName('OSMOND').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectOSMONDDetector 
 
   [26,0,0]
   [26,0,0.0513177]

.. categories::

.. sourcelink::