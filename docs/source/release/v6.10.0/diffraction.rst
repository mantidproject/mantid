===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- ``create_total_scattering_pdf`` in polaris script now creates a total scattering pdf name with a default format that includes the pdf type as a suffix.
- ``create_total_scattering_pdf`` in polaris script now accepts new parameter ``pdf_output_name`` that modifies the output name of the total scattering pdf.

Bugfixes
############
- :ref:`HRPD reduction script <isis-powder-diffraction-hrpd-ref>` now only subtracts empty run from the vanadium when it is subtracted from the sample run (i.e. when ``subtract_empty_instrument=True``) - note other ISIS instrument will always subtract an empty run from the vanadium.
- Algorithm :ref:`WANDPowderReduction <algm-WANDPowderReduction>` no longer displays the button "Replace input workspace", which was considered confusing.


Engineering Diffraction
-----------------------

New features
############
- Fitproperty browser in :ref:`Fitting tab <ui engineering fitting>` of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` for ENGIN-X now includes call to :ref:`algm-FindPeaksConvolve` with good defaults, resulting to add peaks found in the range specified with the sliders on the X axis.
- :ref:`GSAS-II tab <ui engineering gsas>` of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now supports all lattice parameters being overridden (previously was assumed cubic).

Bugfixes
############
- :ref:`Fitting tab <ui engineering fitting>` no longer overwrites blank primary log setting in :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>`  when doing a sequential fit.
- :ref:`Fitting tab <ui engineering fitting>` no longer loads non existing spectra when doing a fit following an ADS clear.
- :ref:`GSAS-II tab <ui engineering gsas>` now uses :ref:`LoadCIF <algm-LoadCIF>` to load phase files which fixes a bug reading the lattice parameters from ``.cif`` files with multiple loop blocks with elements with ``_atom*`` prefix.
- :ref:`GSAS-II tab <ui engineering gsas>` no longer overrides lattice in all phases provided (now only the first phase file lattice is overridden).
- :ref:`GSAS-II tab <ui engineering gsas>` no longer adds Pawley reflections from all phases to each individual phase.


Single Crystal Diffraction
--------------------------

New features
############
- :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` is now significantly faster by using ``scipy.signal.convolve`` instead of ``scipy.ndimag.convolve``.
- :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` no longer has the ``RemoveOnEdge`` option as the algorithm uses new convolution method that ignores edges.
- New algorithm :ref:`FindGoniometerAngles <algm-FindGoniometerAngles>` that does a brute force search for the goniometer rotation angles that maximize the number of peaks indexed by the UB.
- New grouping options using ``HB3AAdjustSampleNorm`` for DEMAND data.
- Diffraction interfaces list now has Garnet in the menu items.
- ``BaseSX`` now has method ``plot_integrated_peaks_MD`` to plot result of IntegratePeaksMD and save in pdf.
- New algorithm :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` to integrate single-crystal Bragg peaks in a workspace with an x-unit of TOF adapted from an algorithm in SXD2001 by Gutmann, M. J. (2005).
- New option to use :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` in ``BaseSX`` class (for use in WISH and SXD reduction).
- New option to find peaks using the ratio of variance/mean in :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` - this is a peak finding criterion used in DIALS software (Winter, G., et al.  Acta Crystallographica Section D: Structural Biology 74.2 (2018): 85-97).
- :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` is the default peak finding algorithm in the SXD reduction class.
- New option to apply ``LorentzCorrection`` to ``ConvertHFIRSCDtoMDE`` for monochromatic single crystal diffraction with rotation about the vertical axis.

Bugfixes
############
- :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` no longer skips peak shapes.
- :ref:`algm-IntegratePeaksShoeboxTOF` no longer throws error when no peak found in vicinity of predicted position.
- :ref:`algm-IntegratePeaksShoeboxTOF` now allows for retrieving shoebox dimensions from strong peaks even when an output file was not specified.

:ref:`Release 6.10.0 <v6.10.0>`
