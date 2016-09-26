.. _FittingMinimizers:

Comparing Minimizers
====================

Minimizers play a central role when :ref:`Fitting a model <Fitting>`
in Mantid. Given the following elements:

- a dataset (e.g. a spectrum),
- a model or function to fit (e.g. a peak or background function, with parameters),
- an initial guess or starting point for the parameters of the function,
- a cost function (e.g., squared residuals (fitting errors) weighted by the
  spectrum errors),
- and a minimizer.

The minimizer is the method that adjusts the function parameters so
that the model fits the data as closely as possible. The cost function
defines the concept of how close a fit is to the data. See the general
concept page on :ref:`Fitting <Fitting>` for a broader discussion of
how these components interplay when fitting a model with Mantid.

Several minimizers are included with Mantid and can be selected in the
`Fit Function property browser
<http://www.mantidproject.org/MantidPlot:_Creating_Fit_Functions>`__
or when using the algorithm :ref:`Fit <algm-Fit>` The following
options are available:

- `Simplex <https://en.wikipedia.org/wiki/Nelder%E2%80%93Mead_method>`__
- `SteepestDescent <https://en.wikipedia.org/wiki/Gradient_descent>`__
- `Conjugate gradient (Fletcher-Reeves imp.) <https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method>`__
- `Conjugate gradient (Polak-Ribiere imp.) <https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method>`__
- `BFGS (Broyden-Fletcher-Goldfarb-Shanno) <https://en.wikipedia.org/wiki/Broyden–Fletcher–Goldfarb–Shanno_algorithm>`__
- `Levenberg-Marquardt <https://en.wikipedia.org/wiki/Levenberg-Marquardt_algorithm>`__ (default)
- Levenberg-MarquardtMD

  A `Levenberg-Marquardt <https://en.wikipedia.org/wiki/Levenberg-Marquardt_algorithm>`__ implementation generalised to allow different cost functions, and supporting chunking techniques for large datasets.
- Damping 

  A `Gauss-Newton <https://en.wikipedia.org/wiki/Gauss–Newton_algorithm#Improved_versions>`__ algorithm with damping.
- :ref:`FABADA <FABADA>`
- A `Trust region <https://ccpforge.cse.rl.ac.uk/gf/project/ral_nlls>`__ algorithm.

All these algorithms are `iterative
<https://en.wikipedia.org/wiki/Iterative_method>`__.  The *Simplex*
algorithm, also known as Nelder–Mead method, belongs to the class of
optimization algorithms without derivatives, or derivative-free
optimization. Note that here simplex refers to downhill simplex
optimization. *Steepest descent* and the two variants of Conjugate
Gradient included with Mantid (*Fletcher-Reeves* and *Polak-Ribiere*)
belong to the class of optimization or minimization algorithms
generally known as conjugate gradient, which use first-order
derivatives. The derivatives are calculated with respect to the cost
function to drive the iterative process towards a local minimum.

BFGS and the Levenberg-Marquardt algorithms belong to the second-order
class of algorithms, in the sense that they use second-order
information of the cost function (second derivatives or the Hessian
matrix). Some algorithms like BFGS approximate the Hessian by the
gradient values of successive iterations. The Levenberg-Marquard
algorithm is a modified Gauss-Newton that introduces an adaptive term
to prevent unstability when the approximated Hessian is not positive
defined. An in-depth description of the methods is beyond the scope of
these pages. More information can be found from the links and general
references on optimization methods such as [Kelley1999]_ and
[NocedalAndWright2006]_.

Finally, :ref:`FABADA <FABADA>` is an algorithm for Bayesian data
analysis. It is excluded from the comparison described below, as it is
a substantially different algorithm.

In most cases, the implementation of these algorithms is based on the
`GSL (GNU Scientific Library) library
https://www.gnu.org/software/gsl/`__, and more specifically on the
`GSL routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__


Comparison of relative goodness of fit and run time
---------------------------------------------------

Here we describe a comparison of minimizers available in Mantid, in
terms of how they perform when fitting several benchmark
problems. This is a relative comparison in the sense that for every
problem the best possible results with Mantid minimizers are given a
top score of "1". The ranking is continuous and the score of a
minimizer represents the ratio between its performance and the
performance of the best. We compare accuracy and run time.

.. Q: accuracy metric: sum of squared residuals or chi2 variants

For example, a ranking of 1.25 for a minimizer for a given problem
means:

- Referring to the accuracy of a minimizer, it produces a solution
  with squared residuals 25% larger than the best solution in Mantid.
- Referring to the run time, it takes 25% more time than the fastest
  minimizer.

All the minimizers available in Mantid 3.7 were compared, with the
exception of FABADA which belongs to a different class of methods and
would not be compared in a fair manner. For all the minimizers
compared here the algorithm :ref:`Fit <algm-Fit>` was run using the
same initialization or starting points for test every problem, as
specified in the test problem definitions. No constraints or ties were
added.

Accuracy is measured using the sum of squared fitting errors as
metric, or "ChiSquared" as defined in :ref:`Fit
<algm-CalculateChiSquared>`, where the fitting errors are the
difference between the expected outputs and the outputs calculated by
the model fitted: :math:`\chi_{1}^{2} = \sum_{i} (y_i - f_i)^2` (see
:ref:`CalculateChiSquared <algm-CalculateChiSquared>` for full details
and different variants).  Run time is measured for the execution of
the :ref:`Fit <algm-Fit>` algorithm for an equation and dataset
previously created.

.. There would be two alternative for the errors:
   1. Without errors, as it is
   2. With "simulated" sqrt(X) errors
   Here we use 2 because Levenberg-Marquardt doesn't work with 1 ('Unweighted least squares' cost function

The cost function used in this general comparison is 'Least squares'
but without using input error estimates (see details below).

Benchmark problems
##################

Each test problem included in this comparison is defined by the
following information:

- Input data (:math:`x_i` values)
- Output data (:math:`y_i` values)
- Function to fit, with parameters
- Starting point or initial values of the function parameters
- Certified or reference best values for the parameters, with an associated residual of the certified or best model 

The problems have been obtained from the following benchmark:

- `NIST nonlinear regression problems <http://itl.nist.gov/div898/strd/general/dataarchive.html>`__.

As the test problems do not define observational errors this
comparison does not use the weights of the least squares cost
function.  An :ref:`alternative comparison using simulated errors is
also available <Minimizers_weighted_comparison>`, with similar results
overall.

Comparison in terms of accuracy
###############################


Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^

The summary table shows the median ranking across all the test
problems. See :ref:`detailed results by test problem (accuracy)
<Minimizers_unweighted_comparison_in_terms_of_accuracy>` (also
accessible by clicking on the cells of the table).

Alternatively, see the :ref:`detailed results when using weighted
least squares as cost function
<Minimizers_weighted_comparison_in_terms_of_accuracy>`.

.. Q: In tables: show values as absolute or relative to the best one
   for each problem? Absolute for the best in a column on the right?
   Relative values simplify this comparison exercise in the sense that
   we don't compare against the NIST reference or similar / one less
   problem

.. summary splitting the NIST problems into three blocks as originally done in the NIST pages

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_acc_summary.txt


Comparison in terms of run time
###############################

Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^

The summary table shows the median ranking across all the test
problems. See :ref:`detailed results by test problem (run time)
<Minimizers_unweighted_comparison_in_terms_of_run_time>`.

Alternatively, see the :ref:`detailed results when using weighted
least squares as cost function
<Minimizers_weighted_comparison_in_terms_of_run_time>`.

.. include:: minimizers_comparison/v3.8.0/comparison_unweighted_v3.8_runtime_summary.txt


Technical details for reproducibility
#####################################

Note that fitting results may be sensitive to the platform and
versions of the algorithms and underlying libraries used.  All the
results shown here have been produced using the same version of Mantid
and on the same system:

- Mantid release 3.8

- Debian 8 GNU/Linux system with an Intel Core i7-4790 processor,
  using `GSL https://www.gnu.org/software/gsl/`__ version 1.16.

References:
             
.. [Kelley1999] Kelley C.T. (1999). Iterative Methods for Optimization.
                SIAM series in Applied Mathematics. Frontiers in Applied
                Mathematics, vol. 18. ISBN: 978-0-898714-33-3.

.. [NocedalAndWright2006] Nocedal J, Wright S. (2006). Numerical Optimization,
                          2nd edition. pringer Series in Operations Research
                          and Financial Engineering.
                          DOI: 10.1007/978-0-387-40065-5

.. categories:: Concepts
