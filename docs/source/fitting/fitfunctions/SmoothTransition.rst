.. _func-SmoothTransition:

=================
Smooth Transition
=================

.. index:: SmoothTransition

Description
-----------

A Smooth Transition fit function is defined as:

.. math:: y = A2\frac{A1-A2}{e^{-\frac{x-M}{G_R}}+1}

where

-  :math:`A1` - the maximum curve
-  :math:`A2` - flat background
-  :math:`M` - the sigmoid midpoint
-  :math:`G_R` - the growth rate

This models a logistic function.

Examples
--------

The logistic function has been used in modelling Covid-19 infection trajectory[1].


.. attributes::

.. properties::

References
----------
[1]Lee, Se Yoon et al. (2020) “Estimation of COVID-19 spread curves integrating global data and borrowing information.” PloS one vol. 15,7  `doi:10.1371/journal.pone.0236860 <https://doi.org/10.1371/journal.pone.0236860>`

.. categories::

.. sourcelink::