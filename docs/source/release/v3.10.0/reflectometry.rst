=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

* :ref:`algm-SpecularReflectionPositionCorrect2 <algm-SpecularReflectionPositionCorrect2> - fixed a bug where entering
  an invalid detector or sample name would cause a segmentation fault.
* The :ref:`algm-SpecularReflectionPositionCorrect` algorithm has a new property, ``DetectorCorrectionType``, which specifies whether detector positions should be corrected by a vertical shift (default) or by a rotation around the sample position.

ConvertToReflectometryQ
-----------------------


Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry
##################

- Interface `ISIS Reflectometry (Polref)` has been renamed to `ISIS Reflectometry`.
- Fixed a bug where the contents of the processing table where not saved to the selected table workspace.

ISIS Reflectometry (Old)
########################

- Interface `ISIS Reflectometry` has been renamed to `ISIS Reflectometry (Old)`.

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.10%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
