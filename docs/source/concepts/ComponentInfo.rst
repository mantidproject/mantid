.. _ComponentInfo:

=============
ComponentInfo 
=============

.. contents::
  :local:

Introduction
------------
``ComponentInfo`` provides faster and simpler access to instrument/beamline geometry as required by Mantid :ref:`Algorithms <Algorithm>` than was possible using :ref:`Instrument`. ``ComponentInfo`` and :ref:`DetectorInfo` are designed as full replacements to :ref:`Instrument`. 

:ref:`Instrument Access Layers <InstrumentAccessLayers>` provides details on how `DetectorInfo` interacts with other geometry access layers.

Python Interface
----------------

Examples of using ``ComponentInfo`` in python

**Print indices of detectors in "bank1" that are masked**

.. testcode:: show_masked_detectors_in_bank 

   from mantid.simpleapi import CreateSampleWorkspace
   
   ws = CreateSampleWorkspace()
   comp_info = ws.componentInfo()
   det_info = ws.detectorInfo()
   det_info.setMasked(2, True) # Mask  a bank 1 detector for demo
   det_info.setMasked(len(det_info)-1, True) # Mask detector not in bank 1
   bank_index = comp_info.indexOfAny('bank1')
   for det_index in comp_info.detectorsInSubtree(bank_index):
       if det_info.isMasked(int(det_index)):
           print('Masked detector index of bank1 is {}'.format(det_index))

Output:

.. testoutput:: show_masked_detectors_in_bank

   Masked detector index of bank1 is 2

.. categories:: Concepts
