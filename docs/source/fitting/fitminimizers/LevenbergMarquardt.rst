.. _LevenbergMarquardt:

Levenberg-Marquardt Minimizer
=============================

This minimizer is explained at - `Wikipedia <https://en.wikipedia.org/wiki/Levenberg-Marquardt_algorithm>`__
It is the default minimizer and is listed in :ref:`a comparison of fitting minimizers <FittingMinimizers Minimizer Comparison>`.

It makes use of the
`GSL (GNU Scientific Library) library
<https://www.gnu.org/software/gsl/>`__, specifically the
`GSL routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__.

.. categories:: FitMinimizers
