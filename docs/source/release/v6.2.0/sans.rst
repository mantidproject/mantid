============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New
---

- :ref:`SANSILLIntegration <algm-SANSILLIntegration>` has new resolution calculation option alternative to Mildner-Carpenter based on fitting horizontal size of direct beam. The fitting is handled in :ref:`SANSILLReduction <algm-SANSILLReduction>` while processing beam.
- :ref:`LoadILLSANS <algm-LoadILLSANS>` is extended to support loading monochromatic kinetic files from the new D11.
- :ref:`SANSILLReduction <algm-SANSILLReduction>` is extended to support processing of monochromatic kinetic runs.
- A series of improvements have been introduced to :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` such as the use of the new :ref:`Stitch <algm-Stitch>`, allowing for semi-automatic q binning, supporting attenuator 2 on D22, as well as improved naming and grouping for the output workspaces.
- ISIS SANS GUI will automatically toggle between Can SAS and NXS Can SAS when switching between 1D and 2D reductions.
  If you have toggled any save options it will not update the selected methods until the interface is restarted to avoid interfering with the user's save selection.
- :ref:`Q1DWeighted <algm-Q1DWeighted>` now supports kinetic data from SANS, with multiple samples instead of time bins.

Bugfixes
--------

- The ISIS SANS Interface will now display an error, instead of an unexpected error, if the user does not have permission to save a CSV file to the requested location.
- The ISIS SANS interface will no longer throw an uncaught exception when a user tries to enter row information without loading a Mask/TOML file.
- The ISIS SANS beam centre finder correctly accepts zero values (0.0) and won't try to replace them with empty strings.

Improvements
############

- :ref:The ANSTO Bilby loader `LoadBBY <algm-LoadBBY>` logs the occurence of invalid events detected in the file as a warning.
- The ISIS SANS threading has been switched to use Python native threading. This provides users with much clearer error messages
  if something goes wrong, and improves tool compatibility for future development.
- :ref:`DeadTimeCorrection <algm-DeadTimeCorrection>` now does not integrate TOF axis if the unit is `Empty`, allowing to correct multi-frame monochromatic SANS data.

:ref:`Release 6.2.0 <v6.2.0>`
