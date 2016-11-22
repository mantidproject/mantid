.. _LevenbergMarquardtMD

Levenberg-Marquardt MD Minimizer
================================

This minimizer uses the same algorithm as the `Levenberg-Marquardt minimizer <LevenbergMarquardt.html>`__ as explained 
in - `Wikipedia <https://en.wikipedia.org/wiki/Levenberg-Marquardt_algorithm>`__ , but is intended for 
`MD workspaces <../concepts/MDWorkspace.html>`__.
It is listed in `a comparison of fitting minimizers <../concepts/FittingMinimizers.html>`__.

It makes use of the 
`GSL (GNU Scientific Library) library
<https://www.gnu.org/software/gsl/>`__, specifically the 
`GSL routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__.

.. categories:: FitMinimizers

