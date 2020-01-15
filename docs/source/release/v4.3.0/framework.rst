=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
############

Concepts
--------

Improvements
############

- Fixed a bug in :ref:`LoadNGEM <algm-LoadNGEM>` where precision was lost due to integer arithmetic.
- Prevent units that are not suitable for :ref:`ConvertUnits <algm-ConvertUnits>` being entered as the target unit.
- Fixed an uncaught exception when plotting logs on single spectrum workspaces in mantidworkbench
- Save the units for single value logs in :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`

Algorithms
----------

Improvements
############

- :ref:`SaveAscii <algm-SaveAscii>` can now save table workspaces, and :ref:`LoadAscii <algm-LoadAscii>` can load them again.
- :ref:`TotScatCalculateSelfScattering <algm-TotScatCalculateSelfScattering>` will calculate a normalized self scattering correction for foccues total scattering data.
- :ref:`MatchAndMergeWorkspaces <algm-MatchAndMergeWorkspaces>` will merge workspaces in a workspace group withing weighting from a set of limits for each workspace and using `MatchSpectra <algm-MatchSpectra>`.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` Sampling of scattering points during MC simulation now takes into account relative volume of sample and environment components.


Data Objects
------------



Geometry
--------

Improvements
############

- Increased numerical accuracy when calculating the bounding box of mili-meter sized cylindrical detector pixels.



Python
------

:ref:`Release 4.3.0 <v4.3.0>`
