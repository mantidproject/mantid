=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

New Features
############

- :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` now supports inelastic instruments (direct and indirect).
  Note that this improvement has involved several breaking changes: the parameter SofqWorkspace has been renamed to StructureFactorWorkspace, the parameter NumberOfWavelengthPoints has been renamed NumberOfSimulationPoints and the EMode parameter has been dropped (with the mode being determined from the workspace instead)
  The algorithm also now requires the input workspace for elastic calculations to have units of Momentum (k) instead of wavelength
- :ref:`LoadAndMerge <algm-LoadAndMerge>` will now offer a possibility to have a joined workspace as output instead of a workspace group.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` will now have a possibility to set a linear integer range as the axis of the output joined workspace.
- :ref:`CalculateFlux <algm-CalculateFlux>` will now work also on workspaces with dimensionless x-axis.
- Performance of :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` improved when running with ImportanceSampling=True
- The :ref:`Fit <algm-Fit>` algorithm now has a property allowing you to calculate the step size between each iteration using the square root of epsilon.
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` now accepts regular expression in BlockList.
- reintroduced :ref:`algm-IntegrateEllipsoids-v1` from release v6.0.0 as v1, marked the latest :ref:`algm-IntegrateEllipsoids-v2` as v2, and created a wrapper that automatically switches between them as a :ref:`algm-IntegrateEllipsoids-v3`
- `Asymmetric Pearson VII <https://docs.mantidproject.org/nightly/fitting/fitfunctions/AsymmetricPearsonVII.html>`_ fit function  can now be chosen as a "ProfileFunction" for the `PoldiDataAnalysis <https://docs.mantidproject.org/nightly/algorithms/PoldiDataAnalysis-v1.html>`_ algorithm.
- the output workspace group that is created by :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` has been updated. The names of the member workspaces are now prefixed by the group name and some additional workspaces have been added to store the integral of the scattering weight across the x axis (Delta E for inelastic and Momentum for elastic)
- The :ref:`algm-ReplaceSpecialValues-v1` algorithm can now operate properly on negative numbers by disabling the UseAbsolute property.

Bugfixes
########

- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` will now get the beam profile from the sample environment in case the sample shape is not defined,
  e.g. in container-only absorption correction calculation with sparse instrument geometry.
- `FileProperty` and `MultiFileProperty` in algorithms now trim whitespaces in the input only when requested (true by default)
- :ref:`LoadAndMerge <algm-LoadAndMerge>` now does not automatically trim whitespaces in the `Filename` property
- :ref:`SmoothNeighbours <algm-SmoothNeighbours>` no longer holds on to its internal input workspace after
  completing execution. This ensures a more prompt removal of the memory it holds when replacing the input workspace
  with the same name.
- the x unit validation on the parameter ScatteringCrossSection in :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` has been corrected to check for Momentum
- Fixed :ref:`algm-Rebin2D` to allow input of :ref:`EventWorkspace <EventWorkspace>` without crashing.

Data Objects
------------

New Features
############



Bugfixes
########



Fit Functions
-------------

New Features
############

* `AsymmetricPearsonVII  <https://docs.mantidproject.org/nightly/fitting/fitfunctions/AsymmetricPearsonVII.html>`_ fit function is implemented in order to improve description of the peaks that have an asymmetric shape.

* New feature: Functions have been extended to allow for Function Attribute Validators. This feature further extends to the FitPropertyBrowser. This allows the value of attributes to be restricted in numerous ways, using validators already available in the Mantid Kernel. Examples of validation include (but are not limited to):

	* A numeric value being bound by a numeric min/max.
	* A string value having to be be selected from a list of possible values.
	* A string value being required to contain specific sub-strings.


Bugfixes
########

- Fixed bug that prevented seeing individual members of composite multi-domain fit functions
- Fix for hard crash when user attempts to use WorkspaceGroup as Workspace in TabulatedFunction. Now, a runtime_error exception is thrown with information that the attribute cannot be assigned.
- Allow the user to put the same values in the `WorkspaceIndex` and `Workspace Index` fields when creating a composite function in the fitting browser.


Python
------

New Features
############

- Added possibility to forward log messages to Python using ``mantid.utils.logging.log_to_python``.
- `setAutoTrim` and `getAutoTrim` from Property class are now exposed to Python API

Bugfixes
########

- Fix a bug where Mantid could hang when performing ADS updates from multiple python algorithms at the same time.

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.


:ref:`Release 6.4.0 <v6.4.0>`
