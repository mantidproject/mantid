=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

Concepts
--------

Algorithms
----------

New Features
############

- The :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` algorithm has been improved.

  - The algorithm now supports inelastic instruments (direct and indirect). **Please Note:** This improvement has involved several breaking changes:

    - The parameter ``SofqWorkspace`` has been renamed to ``StructureFactorWorkspace``.
    - The parameter ``NumberOfWavelengthPoints`` has been renamed ``NumberOfSimulationPoints``.
    - The ``EMode`` parameter has been dropped (the mode is now determined from the workspace instead).
    - The algorithm now requires the input workspace for elastic calculations to have units of momentum (``k``) instead of wavelength.

  - The performance has been improved when ``ImportanceSampling`` is enabled.
  - The output ``WorkspaceGroup`` that is created has been updated. The names of the member workspaces are now prefixed by the group name and some additional
    workspaces have been added to store the integral of the scattering weight across the x axis (Delta E for inelastic and Momentum for elastic)

- :ref:`LoadAndMerge <algm-LoadAndMerge>` now offers a possibility to have a joined workspace as output instead of a workspace group.
- :ref:`ConjoinXRuns <algm-ConjoinXRuns>` can now set a linear integer range as the axis of the output joined workspace.
- :ref:`CalculateFlux <algm-CalculateFlux>` now also works on workspaces with dimensionless x-axis.
- The :ref:`Fit <algm-Fit>` algorithm now has a ``StepSizeMethod`` property, allowing you to calculate the step size between each iteration using the square root of epsilon.
- The :ref:`Asymmetric Pearson VII <func-AsymmetricPearsonVII>` fit function  can now be chosen as a ``ProfileFunction`` for the :ref:`PoldiDataAnalysis <algm-PoldiDataAnalysis-v1>` algorithm.
- :ref:`LoadNexusLogs <algm-LoadNexusLogs>` now accepts a regular expression for the ``BlockList`` property.
- Reintroduced :ref:`algm-IntegrateEllipsoids-v1` from release v6.0.0 as v1, marked the latest :ref:`algm-IntegrateEllipsoids-v2` as v2, and created a wrapper that automatically switches between them as a :ref:`algm-IntegrateEllipsoids-v3`.
- The :ref:`algm-ReplaceSpecialValues-v1` algorithm can now be used to replace negative numbers by disabling the ``UseAbsolute`` property.

Bugfixes
########

- The :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` algorithm will now get the beam profile from the sample environment in case the sample shape is not defined,
  e.g. in a container-only absorption correction calculation with sparse instrument geometry.
- Removal of whitespace characters from ``FileProperty`` and ``MultiFileProperty`` properties in algorithms can now be disabled.
- :ref:`LoadAndMerge <algm-LoadAndMerge>` no longer trims whitespace characters in the ``Filename`` property.
- :ref:`SmoothNeighbours <algm-SmoothNeighbours>` no longer holds on to its internal input workspace after completing execution. This ensures a more prompt removal
  of the memory it holds when replacing the input workspace
  with the same name.
- The x unit validation on the parameter ``ScatteringCrossSection`` in :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` has been corrected to check for momentum.
- :ref:`algm-Rebin2D` can now take an :ref:`EventWorkspace <EventWorkspace>` as an input without crashing.
- Workbench will no longer crash if the algorithm dialog for :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection>` is opened while a group workspace is present in the Workspaces list.
- :ref:`ConvertToPointData <algm-ConvertToPointData>` and :ref:`ConvertToHistogram <algm-ConvertToHistogram>` now handle bin fractions in ``RebinnedOutput`` workspaces by setting all fractions to unity instead of zero. This is because these workspaces are always created as histograms so conversion means discarding the bin information but setting them to zero gives ``NaN``\ s.

Fit Functions
-------------

New Features
############

- An :ref:`AsymmetricPearsonVII  <func-AsymmetricPearsonVII>` fit function has been implemented in order to improve the description of peaks that have an asymmetric shape.

- Functions have been extended to allow for Function Attribute Validators. This feature further extends to the ``FitPropertyBrowser``.
  This allows the value of attributes to be restricted in numerous ways, using validators already available in the Mantid Kernel. Examples of validation include (but are not limited to):

  - A numeric value that is bound by a numeric min/max.
  - A string value that must be one of a list of possible values.
  - A string value that is required to contain specific sub-strings.


Bugfixes
########

- Individual members of composite multi-domain fit functions are now visible.
- Mantid will no longer crash when a ``WorkspaceGroup`` is used in a :ref:`TabulatedFunction <func-TabulatedFunction>`. Instead, a runtime error is shown explaining that the attribute cannot be assigned.
- It is now possible to use the same values for the ``WorkspaceIndex`` and ``Workspace Index`` fields when creating a composite function in the fitting browser.


Kernel
------

Bugfixes
########

- Fixed a bug that prevented the correct inversion of large tridiagonal matrices.


Python
------

New Features
############

- Added possibility to forward log messages to Python using ``mantid.utils.logging.log_to_python``.
- ``setAutoTrim`` and ``getAutoTrim`` from the :py:obj:`~mantid.kernel.Property` class are now exposed to Python.
- ``getNumberDetectors`` from the :py:obj:`~mantid.geometry.Instrument` class is now exposed to the Python API.

Bugfixes
########

- Mantid will no longer hang when performing ADS updates from multiple python algorithms at the same time.

Installation
------------

MantidWorkbench
---------------

See :doc:`mantidworkbench`.


:ref:`Release 6.4.0 <v6.4.0>`
