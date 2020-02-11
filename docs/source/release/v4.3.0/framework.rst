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

- Mantid is now built against Python 3. Windows/macOS bundle Python 3.8 & 3.7 respectively. Ubuntu & Red Hat use system Python 3.6.

Concepts
--------

Improvements
############

- Fixed a bug in :ref:`LoadNGEM <algm-LoadNGEM>` where precision was lost due to integer arithmetic.
- Prevent units that are not suitable for :ref:`ConvertUnits <algm-ConvertUnits>` being entered as the target unit.
- Fixed an uncaught exception when plotting logs on single spectrum workspaces in mantidworkbench
- Save the units for single value logs in :ref:`SaveNexusProcessed <algm-SaveNexusProcessed>`
- Error bars on calculated normalized fits are now correct.

Algorithms
----------

Improvements
############

- :ref:`LoadEventNexus <algm-LoadEventNexus>` Now has a new propery NumberOfBins to select how many linear bins to intially apply to the data.  This allows event data to be plotted immediately without having to :ref:`Rebin <algm-Rebin>` it first.  This is a change from before where event data was initally loaded with a single bin, now by default it will be split into 500 linear bins.
- :ref:`SaveAscii <algm-SaveAscii>` can now save table workspaces, and :ref:`LoadAscii <algm-LoadAscii>` can load them again.
- :ref:`TotScatCalculateSelfScattering <algm-TotScatCalculateSelfScattering>` will calculate a normalized self scattering correction for foccues total scattering data.
- :ref:`MatchAndMergeWorkspaces <algm-MatchAndMergeWorkspaces>` will merge workspaces in a workspace group withing weighting from a set of limits for each workspace and using `MatchSpectra <algm-MatchSpectra>`.
- :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` Sampling of scattering points during MC simulation now takes into account relative volume of sample and environment components. The calculation also now reuses the same set of simulated tracks to calculate the attenuation for different wavelengths. A new parameter ResimulateTracksForDifferentWavelengths has been added to control this behaviour with a default value of false. NOTE: This has been inserted in the middle of the parameter list so any usage of positional parameters with this algorithm will need to be adjusted


Data Objects
------------



Geometry
--------

Improvements
############

- Increased numerical accuracy when calculating the bounding box of mili-meter sized cylindrical detector pixels.



Python
------

- added :py:meth:`mantid.api.Run.getTimeAveragedStd` method to the :py:obj:`mantid.api.Run` object

:ref:`Release 4.3.0 <v4.3.0>`
