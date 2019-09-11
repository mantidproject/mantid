=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------
* :ref:`MaskAngle <algm-MaskAngle>` has an additional option of ``Angle='InPlane'``
* Whitespace is now ignored anywhere in the string when setting the Filename parameter in :ref:`Load <algm-Load>`.
* Added options to :ref:`SaveMD <algm-SaveMD>` to allow selection of what will be saved. For MDHistoWorkspace only.

Data Objects
------------
* New methods :py:obj:`mantid.api.SpectrumInfo.azimuthal` and :py:obj:`mantid.geometry.DetectorInfo.azimuthal`  which returns the out-of-plane angle for a spectrum

Live Data
---------
* Streaming of json geometry has been added to the KafkaLiveListener. User configuration is not required for this.
  The streamer automatically picks up the geometry as a part of the run information and constructs the in-memory geometry without the need for an IDF.

Python
------

API
---

It is now possible to have MultipleFileProperty configured in such a way, that it will allow empty placeholder tokens.

Bug Fixes
---------
* ref:`LoadNexusMonitors <algm-LoadNexusMonitors>` bug fix for user provided top-level NXentry name 

:ref:`Release 4.2.0 <v4.2.0>`

