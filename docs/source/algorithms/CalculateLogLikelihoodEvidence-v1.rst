.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm is used for comparing fitting results from a :ref:`FABADA` fit.
A :ref:`FABADA` fit can output probability distribution profiles for each parameter of the fit and for :math:`\chi^2`.
This algorithm takes in a list of workspaces containing probability distribution profiles for :math:`\chi^2` and computes the Log Likelihood Evidence for each :math:`\chi^2` profile and also outputs the relative Bayes factors between the input :math:`\chi^2` profiles.

The Log Likelihood Evidence of each profile is calculated as:

.. math:: \log \int \exp(-\chi^2/2) P(\chi^2) \, d\chi^2

The relative Log Likelihood between two models:

.. math:: \mathrm{LLE}_1 - \mathrm{LLE}_2

The relative Bayes factors between two models:

.. math:: \exp(\mathrm{LLE}_1 - \mathrm{LLE}_2)

Usage
-----

**Example - CalculateLogLikelihoodEvidence**

In this example we compare the fit of two gaussians against two lorentzians. These two fits have no physical meaning and are purely for demonstration purposes.

.. code-block:: python

  ws_data = Load(Filename='irs26176_graphite002_red.nxs')

  # Linear combination of Lorentzians
  function_str = 'composite=CompositeFunction,NumDeriv=true;name=Lorentzian,Amplitude=1,PeakCentre=0.01,FWHM=0.5;name=Lorentzian,Amplitude=1,PeakCentre=0.01,FWHM=0.5'
  minimizer_str = "FABADA,ChainLength=1000000,StepsBetweenValues=10,ConvergenceCriteria=0.01,PDF=Lorentzians"

  Fit(Function = function_str,InputWorkspace=ws_data,WorkspaceIndex=3,StartX=-0.25,EndX=0.25,CreateOutput=True,Output = 'result_lorentzians',OutputCompositeMembers=True,MaxIterations=2000000, Minimizer=minimizer_str)

  # Linear combination of Gaussians
  function_str = 'composite=CompositeFunction,NumDeriv=true;name=Gaussian, Height=1, PeakCentre=0.01, Sigma=0.5;name=Gaussian, Height=1, PeakCentre=0.01, Sigma=0.5'
  minimizer_str = "FABADA,ChainLength=1000000,StepsBetweenValues=10,ConvergenceCriteria=0.01,PDF=Gaussians"


  Fit(Function = function_str,InputWorkspace=ws_data,WorkspaceIndex=3,StartX=-0.25,EndX=0.25,CreateOutput=True,Output = 'result_gaussians',OutputCompositeMembers=True,MaxIterations=2000000, Minimizer=minimizer_str)

  CalculateLogLikelihoodEvidence(WorkspaceList=["Gaussians_Chi_Squared", "Lorentzians_Chi_Squared"], OutputWorkspace="log_evidence", OutputRelativeFactors="relative_factors")


.. categories::

.. sourcelink::
