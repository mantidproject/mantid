============
SANS Changes
============

.. contents:: Table of Contents
   :local:


New Features
------------

* :ref:`SANSILLIntegration <algm-SANSILLIntegration>` has a new resolution calculation option alternative to Mildner-Carpenter based on fitting horizontal size of direct beam. The fitting is handled in :ref:`SANSILLReduction <algm-SANSILLReduction>` while processing beam.
* :ref:`LoadILLSANS <algm-LoadILLSANS>` is extended to support loading monochromatic kinetic files from the new D11.
* :ref:`SANSILLReduction <algm-SANSILLReduction>` is extended to support processing of monochromatic kinetic runs.
* A series of improvements have been introduced to :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` including

  * the use of the new :ref:`Stitch <algm-Stitch>` algorithm.
  * allowing for semi-automatic q binning.
  * supporting attenuator 2 on D22.
  * improved naming and grouping for the output workspaces.

* The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will automatically toggle between Can SAS and NXS Can SAS when switching between 1D and 2D reductions.
  If you have toggled any save options it will not update the selected methods until the interface is restarted to avoid interfering with the user's saved selection.
* :ref:`Q1DWeighted <algm-Q1DWeighted>` now supports kinetic data from SANS, with multiple samples instead of time bins.


Improvements
------------

- The ANSTO Bilby loader, :ref:`LoadBBY <algm-LoadBBY>`, logs the occurrence of invalid events detected in the file as a warning.
- The ISIS SANS threading has been switched to use Python native threading. This provides users with much clearer error messages
  if something goes wrong, and improves tool compatibility for future development.
- :ref:`DeadTimeCorrection <algm-DeadTimeCorrection>` now does not integrate TOF axis if the unit is `Empty`, allowing to correct multi-frame monochromatic SANS data.
- The ISIS SANS TOML V0 format was updated with several incompatible changes as-per testing feedback.
  A full list of changes can be found on the :ref:`TOML documentation page <sans_toml_v1-ref>` along with more conversion entries.
- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` now uses Front / Rear instead of HAB / LAB respectively. This only affects the text in the interface, and does
  not affect compatibility with existing user files.

Bugfixes
--------

- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will now display an error, instead of an unexpected error, if the user does not have permission to save a CSV file to the requested location.
- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will no longer throw an uncaught exception when a user tries to enter row information without loading a Mask/TOML file.
- The :ref:`ISIS SANS beam centre finder<ISIS_SANS_Beam_Centre_Tab-ref>` correctly accepts zero values (0.0) and won't try to replace them with empty strings.
- The warning ``Reduction Mode 'x' is not valid`` will no longer incorrectly show when there are errors with the user's mask file.


:ref:`Release 6.2.0 <v6.2.0>`
