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

PDF (*required*)
  Probability Density Function for each fitted parameter and the cost function.
  This is output as a :ref:`MatrixWorkspace`.

Chains (*optional*)
  The value of each parameter and the cost function for each step taken.
  This is output as a :ref:`MatrixWorkspace`.

ConvergedChain (*optional*)
  A subset of Chains containing only the section after which the parameters have
  converged.
  This records the parameters at step intervals given by StepsBetweenValues.
  This is output as a :ref:`MatrixWorkspace`.

CostFunctionTable (*optional*)
  Table containing the minimum and most probable values of the cost function as
  well as their reduced values.
  This is output as a TableWorkspace.

Parameters (*optional*)
  Similar to the standard parameter table but also includes left and right
  errors for each parameter (cost function is not included).
  This is output as a TableWorkspace.

Usage
-----

**Example: A simple example**

.. code-block:: python

  ws_data = Load(Filename='irs26176_graphite002_red.nxs')
  ws_res = Load(Filename='irs26173_graphite002_res.nxs')

  function_str = 'composite=Convolution,FixResolution=tue,NumDeriv=false;name=Resolution,Workspace=ws_res,WorkspaceIndex=0;(composite=CompositeFunction,NumDeriv=true;name=Lorentzian,Amplitude=1,PeakCentre=0.01,FWHM=0.5;name=Lorentzian,Amplitude=1,PeakCentre=0.01,FWHM=0.5)'
  minimizer_str = "FABADA,Chain Length=1000000,Steps between values=10,Convergence Criteria=0.01,PDF=pdf,Chains=chain,Converged chain=conv,Cost Function Table=CostFunction,Parameter Erros =Errors"

  Fit(Function = function_str,InputWorkspace=ws_data,WorkspaceIndex=3,StartX=-0.25,EndX=0.25,CreateOutput=True,Output = 'result',OutputCompositeMembers=True,MaxIterations=2000000, Minimizer=minimizer_str)


.. categories:: Concepts
