=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
############################

**The old ISIS Reflectometry Interface has been removed. If required then please use the previous release, version 4.2.0**

Improved
--------

- The per-angle defaults table on the Experiment now has column-specific tooltips on the table cells which correspond to the [ReflectometryISISLoadAndProcess](https://docs.mantidproject.org/nightly/algorithms/ReflectometryISISLoadAndProcess-v1.html?highlight=reflectometryisisloadandprocess) documentation

Algorithms
##########

New
---

- The new workflow algorithm :ref:`algm-ReflectometryILLAutoProcess` performs the complete data reduction for ILL reflectometers D17 and Figaro. Implements both coherent and incoherent summation types.

- Options to perform background subtraction have been added to :ref:`algm-ReflectometryReductionOne`, :ref:`algm-ReflectometryReductionOneAuto` and :ref:`algm-ReflectometryISISLoadAndProcess`. This uses the :ref:`algm-ReflectometryBackgroundSubtraction` to perform the subtraction using one of three possible methods.

Bug fixes
---------

- The detector is now correctly position for fractional line position inputs for :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>`.
- The `TwoTheta` input option works now correctly for :ref:`ReflectometryILLPreProcess <algm-ReflectometryILLPreProcess>`


:ref:`Release 4.3.0 <v4.3.0>`
