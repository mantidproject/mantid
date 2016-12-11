.. _DampedGaussNewton:

Damped Gauss-Newton minimizer
=============================

This minimizer is
explained at `Wikipedia <https://en.wikipedia.org/wiki/Gauss–Newton_algorithm#Improved_versions>`__ 
and has damping.

It is listed in `a comparison of fitting minimizers <../concepts/FittingMinimizers.html>`__.

It makes use of the 
`GSL (GNU Scientific Library) library
<https://www.gnu.org/software/gsl/>`__, specifically the 
`GSL routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__.

.. categories:: FitMinimizers

