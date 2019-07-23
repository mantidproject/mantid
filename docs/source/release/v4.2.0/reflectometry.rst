=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
----------------------------

Improved
########

- The polarization correction inputs have been simplified to a single checkbox which when ticked will apply polarization corrections based on properties in the instrument parameters file.

Algorithms
----------

New
###

- :ref:`ReflectometryReductionOneAuto <algm-ReflectometryReductionOneAuto-v3>` has been rewritten and updated to version 3. In the new version the polarization correction properties have been removed from the algorithm input and are now taken from the parameter file. A checkbox has been added to indicate whether the corrections should be applied.

:ref:`Release 4.2.0 <v4.2.0>`
