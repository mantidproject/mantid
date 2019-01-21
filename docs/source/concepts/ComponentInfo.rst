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

Print indices of detectors in "bank1" that are masked

.. code-block:: python 

  from mantid.simpleapi import CreateSampleWorkspace

  ws = CreateSampleWorkspace()
  comp_info = ws.componentInfo()
  det_info = ws.detectorInfo()
  bank_index compinfo.indexOfAny('bank1')
  for det_index in compinfo.detectorsInSubtree(bank_index):
      if det_info.isMasked(int(det_index)):
          print('Masked detector of bank1, ', det_index)

