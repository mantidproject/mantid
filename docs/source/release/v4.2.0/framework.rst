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

Data Objects
------------

Live Data
---------
* Streaming of json geometry has been added to the KafkaLiveListener. User configuration is not required for this.
  The streamer automatically picks up the geometry as a part of the run information and constructs the in-memory geometry without the need for an IDF.

Python
------

Bug Fixes
---------
* ref:`LoadNexusMonitors <algm-LoadNexusMonitors>` bug fix for user provided top-level NXentry name 

:ref:`Release 4.2.0 <v4.2.0>`

