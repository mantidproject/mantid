=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the F(Q)Fit tab, Multiple Input tab that allowed duplicate spectra to be added.

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New Features
############

- Fit functions `ElasticIsoRotDiff` and `InelasticIsoRotDiff` have been made available in the ConvFit tab in the Indirect Data Analysis
- The Abins Algorithm can now import XML data from VASP calculations
  using "selective dynamics" to restrict the set of atoms active in
  vibrations. The data is imported and processed as though these are
  the only atoms in the system, with appropriately-dimensioned
  displacement data. This approximation is useful for the study of
  light (e.g. organic) molecules adsorbed to surfaces of heavy
  (e.g. noble-metal) catalysts.

Bug Fixes
#########

- A bug has been fixed that stopped additional spectra to be added to Indirect Data Analysis if spectra from that workspace had already been added.

Improvements
############

- Single input has been removed from the Indirect Data Analysis Fit tabs. All data input is now done via the multiple input dialog.
- The data input widgets in the Indirect Data Analysis fit tabs has been made dockable and can be resized once undocked.
- Introduced multithreading for detectors/spectra to VesuvioCalculateMS in order to speed up the VesuvioAnalysis algorithm.

:ref:`Release 6.2.0 <v6.2.0>`
