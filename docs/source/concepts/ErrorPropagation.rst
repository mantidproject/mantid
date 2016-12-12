.. _Error Propagation:

Error Propagation
=================

The purpose of this document is to explain how Mantid deals with error
propagation and how it is used in its algorithms.

Theory
------

In order to deal with error propagation, Mantid treats errors as Gaussian
probabilities (also known as a bell curve or normal probabilities) and each
observation as independent. Meaning that if :math:`X = 100 \pm 1` then it is still
possible for a value of :math:`102` to occur, but less likely than :math:`101`
or :math:`99`, and a value of :math:`105` is far less likely still than any of
these values.

Plus and Minus Algorithm
------------------------

The :ref:`algm-Plus` algorithm adds two datasets together, propagating the
uncertainties. Mantid calculates the result of :math:`X_1 + X_2` as

:math:`X = X_1 + X_2`

with uncertainty

:math:`\sigma_X = \sqrt{ \left( \sigma_{X_1} \right)^2 + \left( \sigma_{X_2} \right)^2 }`.

Consider the example where :math:`X_1 = 101 \pm 2` and :math:`X_2 = 99 \pm 2`.
Then for this algorithm:

:math:`X = X_1 + X_2 = 101 + 99 = 200`

:math:`\sigma_X = \sqrt{ 2^2 + 2^2} = \sqrt{8} = 2.8284`

Hence the result of :ref:`algm-Plus` can be summarised as :math:`X = 200 \pm \sqrt{8}`.

Mantid deals with the :ref:`algm-Minus` algorithm similarly: the result of :math:`X_1 - X_2` is

:math:`X = X_1 - X_2`

with error

:math:`\sigma_X = \sqrt{ \left( \sigma_{X_1} \right)^2 + \left( \sigma_{X_2} \right)^2 }`.

Multiply and Divide Algorithm
-----------------------------

The :ref:`algm-Multiply` and :ref:`algm-Divide` algorithms propagate the uncertainties according
to (see also `here <http://en.wikipedia.org/wiki/Propagation_of_uncertainty>`_):

:math:`\sigma_X = \left|X\right| \sqrt{ \left( \frac{\sigma_{X_1}}{X_1} \right)^2 + \left( \frac{\sigma_{X_2}}{X_2} \right)^2 }`,

where :math:`X` is the result of the multiplication, :math:`X = X_1 \cdot X_2`, or the division, :math:`X = X_1 / X_2`.

Considering the example above where :math:`X_1 = 101 \pm 2` and
:math:`X_2 = 99 \pm 2`. Mantid would calculate the result of :math:`X_1 / X_2` as
:math:`X = 101 / 99 = 1.0202`, with uncertainty
:math:`\sigma_X = 1.0202 \sqrt{ \left(2/101\right)^2 + \left(2/99\right)^2} = 0.0288`.

For :ref:`algm-Multiply`, the result of :math:`X_1 \times X_2` is
:math:`X = 101 \times 99 = 9999`, with uncertainty
:math:`\sigma_X = 9999 \sqrt{ \left(2/101\right)^2 + \left(2/99\right)^2} = 282.8568`.


.. categories:: Concepts
