=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
############
- Fit functions `ElasticIsoRotDiff` and `InelasticIsoRotDiff` have been made available in the ConvFit tab in the Indirect Data Analysis
- A new option "Autoconvolution" is added to the Abins Algorithm.
  This enables simulation of quantum orders up to order 10 by
  convolving the highest calculated spectrum (order 1 or 2) against
  the fundamentals spectrum repeatedly, before applying Debye-Waller
  terms. (NB: This has introduced small numerical differences from
  previous versions of Abins, because data is now binned before
  applying Debye-Waller terms. This difference will converge with
  small bin sizes.)

Improvements
############
- Single input has been removed from the Indirect Data Analysis Fit tabs. All data input is now done via the multiple input dialog.
- The data input widgets in the Indirect Data Analysis fit tabs has been made dockable and can be resized once undocked.
- Introduced multithreading for detectors/spectra to VesuvioCalculateMS in order to speed up the VesuvioAnalysis algorithm.
- The Elwin tab in Indirect Data Analysis has a new loader which now allows users to add workspaces.
- The Abins Algorithm can now import XML data from VASP calculations
  using "selective dynamics" to restrict the set of atoms active in
  vibrations. The data is imported and processed as though these are
  the only atoms in the system, with appropriately-dimensioned
  displacement data. This approximation is useful for the study of
  light (e.g. organic) molecules adsorbed to surfaces of heavy
  (e.g. noble-metal) catalysts.
- Abins: Thresholding of low-intensity modes has been changed. This
  impacts the second-order spectrum, especially at elevated
  temperature; excitations were being discarded on the basis of a low
  intensity in the fundamental spectrum, when they could contribute to a
  noticable peak in the second-order spectrum.

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the F(Q)Fit tab, Multiple Input tab that allowed duplicate spectra to be added.
- A bug has been fixed that stopped additional spectra being added to Indirect Data Analysis if spectra from that workspace had already been added.


:ref:`Release 6.2.0 <v6.2.0>`
