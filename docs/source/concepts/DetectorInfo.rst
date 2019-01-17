.. _DetectorInfo:

=============
DetectorInfo
=============

.. contents::
  :local:

Introduction
------------
``DetectorInfo`` provides faster and simpler access to instrument/beamline detector geometry and metadata as required by Mantid :ref:`Algorithms <Algorithm>` than was possible using :ref:`Instrument`. ``DetectorInfo`` and :ref:`ComponentInfo` are designed as full replacements to :ref:`Instrument`. 

:ref:`Instrument Access Layers <InstrumentAccessLayers>` provides details on how `DetectorInfo` interacts with other geometry access layers.

Python Interface
----------------

Examples of using ``DetectorInfo`` in python

Mask detectors at some distance from the source

.. code-block:: python 

  from mantid.simpleapi import CreateSampleWorkspace
  from mantid.geometry import DetectorInfo, DetectorInfoPythonIterator

  # Test workspace with instrument
  ws = CreateSampleWorkspace()
  det_info = ws.detectorInfo();
  for item in det_info:
      if not item.isMonitor and item.l2 > 2.0:
          item.setMasked(True)

Print detectors with scattering angle

.. code-block:: python 

  from mantid.simpleapi import CreateSampleWorkspace
  from mantid.geometry import DetectorInfo, DetectorInfoPythonIterator

  # Test workspace with instrument
  ws = CreateSampleWorkspace()
  det_info = ws.detectorInfo()
  for item in det_info:
      if item.l2 > 0.01:
         print(item.index) 
