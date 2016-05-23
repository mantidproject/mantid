.. _FittingMinimizers:

Comparing Minimizers
====================

Minimizers play a central role when :ref:`Fitting a model <Fitting>`
in Mantid. Given the following elements:

- a dataset (e.g. a spectrum),
- a model or function to fit (e.g. a peak or background function, with parameters),
- an initial guess or starting point for the parameters of the function,
- a cost function (e.g., squared errors weighted by the spectrum errors),
- and a minimizer,

The minimizer is the algorith or method that adjusts the function
parameters so that the model fits the data as closely as possible. The
cost function defines the concept of how close a fit is to the
data. See the general concept page on :ref:`Fitting <Fitting>` for a
broader discussion of how these components interplay when fitting a
model with Mantid.

Several minimizers are included with Mantid and can be seleced in the
`Fit Function property browser
<http://www.mantidproject.org/MantidPlot:_Creating_Fit_Functions>`__
or when using the algorithm :ref:`Fit <algm-Fit>` The following
options are available:

- `Simplex <https://en.wikipedia.org/wiki/Nelder%E2%80%93Mead_method>`__
- `SteepestDescent <https://en.wikipedia.org/wiki/Gradient_descent>`__
- `Conjugate gradient (Fletcher-Reeves imp.) <https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method>`__
- `Conjugate gradient (Polak-Ribiere imp.) <https://en.wikipedia.org/wiki/Nonlinear_conjugate_gradient_method>`__
- `BFGS (Broyden-Fletcher-Goldarfb-Shanno) <https://en.wikipedia.org/wiki/Broyden–Fletcher–Goldfarb–Shanno_algorithm>`__
- `Levenberg-Marquardt <https://en.wikipedia.org/wiki/Levenberg-Marquardt_algorithm>`__ (default)
- Levenberg-MarquardtMD
- Damping
- :ref:`FABADA <FABADA>`

**TODO: Missing info: *Damping* differences between Lev-Mar and Lev-Mar "MD"**.

All these algorithms are `iterative
<https://en.wikipedia.org/wiki/Iterative_method>`__.  The *Simplex*
algorithm, also known as Nelder–Mead method, belongs to the class of
optimization algorithms without derivatives, or derivative-free
optimization (another example of this class would be simulated
annealing). Note that here simplex refers to downhill simplex
optimization. *Steepest descent* and the two variants of Conjugate
Gradient included with Mantid (*Fletcher-Reeves* and *Polak-Ribiere*)
belong to the class of optimization or minimization algorithms
generally known as conjugate gradient, which use first-order
derivatives. The derivatives are calculated on the error of the fit to
drive the iterative process towards a local minimum.

BFGS and the Levenberg-Marquardt algorithms belong to the second-order
class of algorithms, in the sense that they use second-order
information of the error function (second derivatives or the Hessian
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

In most cases, the implementations of these algorithms are based on
the `GNU Scientific Libraty routines for least-squares fitting
<https://www.gnu.org/software/gsl/manual/html_node/Least_002dSquares-Fitting.html#Least_002dSquares-Fitting>`__

**TODO: Exceptions that are not GSL methods: Levenberg-MarquardMD**.


Comparison of relative goodness of fit and run time
---------------------------------------------------

Relative comparison of minimizers available in Mantid.

The best possible results with Mantid minimizers are given a top score
"1". The ranking is continuous and the score of a minimizer represents
the ratio between its performance and the performance of the best.

.. Q: accuracy metric: sum of squared residuals or chi2 variants

Accuracy, using as metric the sum of squared errors.

Run time. Each fitting problem takes a small fraction of a second. For
robustness, N repetitions.

For example, a ranking of 1.25 for a given problem means:

- Referring to the accuracy of a minimizer, it produces a solution
  with squared residuals 25% larger than the best solution in Mantid.
- Referring to the run time, it takes 25% more time than the fastest
  minimizer.

Minimizers used as "black boxes", without any special initialization,
constraints, rewriting of the equations to fit.

Cost function used: `Least squares` (weighted).

The difference between the expected outputs and the outputs calculated
by the model fitted: :math:`\chi_{1}^{2} = \sum_{i} (y_i - f_i)^2`
(see :ref:`CalculateChiSquared <algm-CalculateChiSquared>` for full
details and different variants).

.. There would be two alternative for the errors:
   1. Without errors, as it is
   2. With "simulated" sqrt(X) errors
   Here we use 2 because Levenberg-Marquardt doesn't work with 1 ('Unweighted least squares' cost function

All the minimizers available in Mantid 3.7, with the exception of
FABADA which belongs to a different class of methods and could not be
compared here in a fair manner.

Benchmark problems
##################

Each test problem included in this benchmark is defined by the
following information:

- Input data (:math:`x_i` values)
- Output data (:math:`y_i` values)
- Function to fit, with parameters
- Starting point or initial values of the function parameters
- Certified or reference best values for the parameters, with an associated residual of the certified or best model 

The problems have been obtained from two different sources:

- `NIST nonlinear regression problems <http://itl.nist.gov/div898/strd/general/dataarchive.html>`__.

- The `Constrained and Unconstrained Testing Environment <http://www.cuter.rl.ac.uk/Problems/mastsif.html>`__
  set of test problems for linear and nonlinear optimization.

From the second source, which is a large collection of diverse
problems (over a thousand) a subset of problems that are relevant for
fitting in Mantid was selected. These problems are unconstrained, with
a sum of squared objective function, and regular (first and second
derivatives exist and are continuous everywhere), following the `CUTE
classification scheme
<http://www.cuter.rl.ac.uk/Problems/classification.shtml>`__ for
optimization test problems.


Comparison in terms of accuracy
###############################


Summary
^^^^^^^

The summary table shows the median ranking across all the test
problems. See :ref:`detailed results by test problem (accuracy)
<Minimizers_comparison_in_terms_of_accuracy>`.


.. Q: In tables: show values as absolute or relative to the best one
   for each problem? Absolute for the best in a column on the right?
   Relative values simplify this comparison exercise in the sense that
   we don't compare against the NIST reference or similar / one less
   problem

.. summary splitting the NIST problems into three blocks as originally done in the NIST pages

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_acc_summary.txt


Comparison in terms of run time
###############################

Summary, median ranking
^^^^^^^^^^^^^^^^^^^^^^^

The summary table shows the median ranking across all the test
problems. See :ref:`detailed results by test problem (run time)
<Minimizers_comparison_in_terms_of_run_time>`.

.. include:: minimizers_comparison/v3.7.0/comparison_v3.7_runtime_summary.txt


References:
             
.. [Kelley1999] Kelley C.T. (1999). Iterative Methods for Optimization.
                SIAM series in Applied Mathematics. Frontiers in Applied
                Mathematics, vol. 18. ISBN: 978-0-898714-33-3.

.. [NocedalAndWright2006] Nocedal J, Wright S. (2006). Numerical Optimization,
                          2nd edition. pringer Series in Operations Research
                          and Financial Engineering.
                          DOI: 10.1007/978-0-387-40065-5

.. categories:: Concepts
