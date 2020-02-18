=====================
Reflectometry Changes
=====================

ISIS Reflectometry Interface
############################

Improved
--------

- The per-angle defaults table on the Experiment now has column-specific tooltips on the table cells.

Algorithms
##########

New
---

- The new workflow algorithm :ref:`algm-ReflectometryILLAutoProcess` performs the complete data reduction for ILL reflectometers D17 and Figaro. Implements both coherent and incoherent summation types.

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Bug fixes
---------

- The detector is now correctly position for fractional line position inputs for :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.
- The `TwoTheta` input option works now correctly for :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>`


:ref:`Release 4.3.0 <v4.3.0>`
