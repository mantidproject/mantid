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

:math:`(\mbox{A0}+\mbox{A1}\times x) \times  \mbox{Height}\times \exp(-\frac{x}{\mbox{Lifetime}})`

This function may be used with the :ref:`algm-Fit` algorithm. However, it
was originally added to Mantid as a named function for the purposes of
detector efficiency calibration.

.. attributes::

.. properties::

.. categories::
