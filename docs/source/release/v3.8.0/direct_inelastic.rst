========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

Improvements
------------

- :ref:`MDNormDirectSC <algm-MDNormDirectSC>` has an option to skip safety checks. This improves the speed when acting on workspace groups.

- :ref:`MaskDetectors <algm-MaskDetectors>` modified to work on a grouped workspace, so if a spectra of the mask workspace is masked, the 
 spectra of the target workspace, with the detector group containing the masked detector become masked. This allows to use *.xml* masks, prodiced by 
 :ref:`SaveMask <algm-SaveMask>` algorithm on the grouped workspaces, obtained from ISIS instruments. 

`Full list of changes on GitHub <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Direct+Inelastic%22>`_

Crystal Field
-------------

- A fitting function was added (:ref:`CrystalFieldMultiSpectrum <func-CrystalFieldMultiSpectrum>`) that fits crystal field parameters to multiple spectra simultaneously.

