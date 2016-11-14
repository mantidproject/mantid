=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Algorithms
----------

* New versions of algorithms :ref:`algm-ReflectometryReductionOne` and :ref:`algm-CreateTransmissionWorkspace`
  have been added to remove duplicate steps in the reduction. An improvement in performance of factor x3 has
  been observed when the reduction is performed with no monitor normalization.

* New versions of :ref:`algm-ReflectometryReductionOneAuto`, :ref:`algm-CreateTransmissionWorkspaceAuto` and
  :ref:`algm-SpecularReflectionPositionCorrect` have been added. The new versions fix the following known issues:

  * When :literal:`CorrectionAlgorithm` was set to :literal:`AutoDetect` the algorithm was not able to find polynomial
    corrections, as it was searching for :literal:`polynomial` instead of :literal:`polystring`.
  * Fix some problems when moving the detector components in the instrument. The new version uses :literal:`ProcessingInstructions`
    to determine which detector components need to be moved.
  * Monitor integration range was not being applied properly to CRISP data. The problem was that in the parameter
    file the wavelength range used to crop the workspace in wavelength is [0.6, 6.5] and the monitor integration range is outside of these limits ([4, 10]). This was causing the algorithm to integrate over [0.6, 4].

Reflectometry Reduction Interface
---------------------------------

ISIS Reflectometry (Polref)
###########################

ISIS Reflectometry
##################

|

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.9%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
