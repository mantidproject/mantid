=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
###

- The new work-flow algorithm :ref:`algm-ReflectometryILLAutoProcess` allows to perform a flexible reduction. It implements the incoherent reduction and the sample angle option.

BUGFIXES
########

- The detector is now correctly position for fractional line position inputs for :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.
- The `TwoTheta` input option works now correctly for :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>`


:ref:`Release 4.1.0 <v4.1.0>`
