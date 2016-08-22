.. _func-ProductLinearExp:

================
ProductLinearExp
================

.. index:: ProductLinearExp

Description
-----------

This fit function computes the product of a linear and exponential
function. See `ExpDecay <ExpDecay>`__ and
`LinearBackground <LinearBackground>`__ for details on the component
functions.

.. math:: \left(\mbox{A}_0 + \mbox{A}_1 x\right)\cdot h\cdot e^{-\frac{x}{\tau}}

This function may be used with the :ref:`algm-Fit` algorithm. However, it
was originally added to Mantid as a named function for the purposes of
detector efficiency calibration.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
