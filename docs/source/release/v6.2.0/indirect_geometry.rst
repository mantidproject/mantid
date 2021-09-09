=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
############
- Fit functions :ref:`ElasticIsoRotDiff <func-ElasticIsoRotDiff>` and :ref:`InelasticIsoRotDiff <func-InelasticIsoRotDiff>` have been made available in the ConvFit tab in the :ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>` interface.
- - **The** :ref:`Elwin <Elwin-iqt-ref>` **tab in :ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>` has a new loader which now allows users to add workspaces.**

.. figure:: ../../images/ElwinLoad.gif
   :width: 800px
   :align: center

- A new option "Autoconvolution" is added to the :ref:`Abins Algorithm <algm-Abins>`.
  This enables simulation of quantum orders up to order 10 by convolving the highest calculated spectrum (order 1 or 2) against
  the fundamentals spectrum repeatedly, before applying Debye-Waller terms. (NB: This has introduced small numerical differences from
  previous versions of Abins, because data is now binned before applying Debye-Waller terms. This difference will converge with
  small bin sizes.)

Improvements
############
- Single input has been removed from the :ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>` Fit tabs. All data input is now done via the multiple input dialog.
- The data input widgets in the :ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>` Fit tabs has been made dockable and can be resized once undocked.
- Introduced multithreading for detectors/spectra to :ref:`VesuvioCalculateMS <algm-VesuvioCalculateMS>` in order to speed up the :ref:`VesuvioAnalysis <algm-VesuvioAnalysis>` algorithm.
- The :ref:`Abins Algorithm <algm-Abins>` can now import XML data from VASP calculations using "selective dynamics" to restrict the set of atoms active in
  vibrations. The data is imported and processed as though these are the only atoms in the system, with appropriately-dimensioned
  displacement data. This approximation is useful for the study of light (e.g. organic) molecules adsorbed to surfaces of heavy (e.g. noble-metal) catalysts.
- :ref:`Abins Algorithm <algm-Abins>`: Thresholding of low-intensity modes has been changed. This
  impacts the second-order spectrum, especially at elevated temperature; excitations were being discarded on the basis of a low
  intensity in the fundamental spectrum, when they could contribute to a noticable peak in the second-order spectrum.

Bugfixes
########
- A bug has been fixed in Indirect data analysis on the F(Q)Fit tab, Multiple Input tab that allowed duplicate spectra to be added.
- A bug has been fixed that stopped additional spectra being added to Indirect Data Analysis if spectra from that workspace had already been added.
- :ref:`IndirectILLEnertyTransfer <algm-IndirectILLEnertyTransfer>` will now perform the monitor normalisation correctly; that is, in wavelength instead of energy. It will also provide the monitor workspace as a diagnostic output, if requested.
- A bug has been fixed in Indirect Data Analysis that limited the integration range on the tabs to between -1 and 1.


:ref:`Release 6.2.0 <v6.2.0>`
