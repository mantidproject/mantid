.. _PolakRiberiere:

Conjugate Gradient Minimizer (Polak-Ribiere imp.)
=================================================

This minimizer an implementation of the nonlinear conjugate gradient method 
explained at `Wikipedia <https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method>`__ .

It is listed in `a comparison of fitting minimizers <../concepts/FittingMinimizers.html>`__.

It makes use of the 
`GSL (GNU Scientific Library) library
<https://www.gnu.org/software/gsl/>`__, specifically the 
`GSL routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__.

.. categories:: FitMinimizers
