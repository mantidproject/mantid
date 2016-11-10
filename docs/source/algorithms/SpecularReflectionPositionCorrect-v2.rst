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

   offspec = Load(Filename=r'OFFSPEC00010791.raw', PeriodList=1)
   offspec = offspec[0]

   instr = offspec.getInstrument()
   print instr.getComponentByName('point-detector').getPos()

   offspec = SpecularReflectionPositionCorrect(offspec, TwoTheta = 2*0.3, DetectorComponentName='point-detector')
   instr = offspec.getInstrument()
   print instr.getComponentByName('point-detector').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectPointDetector 
 
   [0,0,3.19]
   [0,0.0334068,3.19]

**Example - Correct 'LinearDetector'**

.. testcode:: SpecularReflectionPositionCorrectLinearDetector

   offspec = Load(Filename=r'OFFSPEC00010791.raw', PeriodList=1)
   offspec = offspec[0]

   instr = offspec.getInstrument()
   print instr.getComponentByName('LinearDetector').getPos()

   offspec = SpecularReflectionPositionCorrect(offspec, TwoTheta = 2*0.3, DetectorComponentName='LinearDetector')
   instr = offspec.getInstrument()
   print instr.getComponentByName('LinearDetector').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectLinearDetector 
 
   [0,0,3.65]
   [0,0.0382241,3.65]

**Example - Correct 'WLSFDetector'**

.. testcode:: SpecularReflectionPositionCorrectWLSFDetector

   offspec = Load(Filename=r'OFFSPEC00010791.raw', PeriodList=1)
   offspec = offspec[0]

   instr = offspec.getInstrument()
   print instr.getComponentByName('WLSFDetector').getPos()

   offspec = SpecularReflectionPositionCorrect(offspec, TwoTheta = 2*0.3, DetectorComponentName='WLSFDetector')
   instr = offspec.getInstrument()
   print instr.getComponentByName('WLSFDetector').getPos()
   
Output:

.. testoutput:: SpecularReflectionPositionCorrectWLSFDetector 
 
   [0,0,3.62]
   [0,0.0379099,3.62]

.. categories::

.. sourcelink::