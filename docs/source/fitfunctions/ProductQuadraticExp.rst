.. _func-ProductQuadraticExp:

===================
ProductQuadraticExp
===================

.. index:: ProductQuadraticExp

Description
-----------

This fit function computes the product of a linear and exponential
function. See `ExpDecay <ExpDecay>`__ and QuadraticBackground for
details on the component functions.

.. math:: \left(\mbox{A}_0 + \mbox{A}_1 x + \mbox{A}_2 x^2\right)\cdot h \cdot e^{-\frac{x}{\tau}}

This function may be used with the :ref:`algm-Fit` algorithm. However, it
was originally added to Mantid as a named function for the purposes of
detector efficiency calibration. Also see
`ProductLinearExp <func-ProductLinearExp>`__.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
