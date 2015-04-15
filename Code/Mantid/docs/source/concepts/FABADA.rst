.. _FABADA:

FABADA
======

FABADA is a fitting algorithm for Bayesian data analysis, the theory of which is
detailed here: http://dx.doi.org/10.1088/1742-6596/325/1/012006

This documentation covers details of it's implementation in Mantid.

Important Notes
---------------

Currently in order to use FABADA the cost function must be set to Least Squares.

The values for ChainLength and MaxIterations should be set to relatively high
values where :math:`MaxIterations > 2 * ChainLength` (e.g. MaxIterations = 20^6
and ChainLength = 10^6).

Currently the starting values used for each parameter are required to be a
fairly good estimate of the actual value, in the event that a parameter is not
estimated to sufficient accuracy convergence of the parameters will not be
reached and an error message will inform of the unconverged parameters.

FABADA Specific Parameters
--------------------------

ChainLength
  Number of steps done by the algorithm once all of the parameters have
  converged.

StepsBetweenValues
  Specifies interval at which to record parameter and cost function values in
  the chain outputs.

ConvergenceCriteria
  The threshold variation in the cost function due to the change in a parameter
  under which it is assumed that the parameter has reached convergence.

JumpAcceptanceRate
  The desired percentage of acceptance for new parameters (typically 0.666)

FABADA Specific Outputs
-----------------------

PDF
  Probability Density Function for each fitted parameter and the cost function.
  This is output as a :ref:`MatrixWorkspace`.

Chains
  The value of each parameter and the cost function for each step taken.
  This is output as a :ref:`MatrixWorkspace`.

ConvergedChain
  A subset of Chains containing only the section after which the parameters have
  converged.
  This records the parameters at step intervals given by StepsBetweenValues.
  This is output as a :ref:`MatrixWorkspace`.

CostFunctionTable
  Table containing the minimum and most probable values of the cost function as
  well as their reduced values.
  This is output as a TableWorkspace.

Parameters
  Similar to the standard parameter table but also includes left and right
  errors for each parameter (cost function is not included).
  This is output as a TableWorkspace.

.. categories:: Concepts
