===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- The default format of the total scattering pdf name created by ``create_total_scattering_pdf`` in polaris script has been changed to include the pdf type as a suffix
- Add a new parameter ``pdf_output_name`` to ``create_total_scattering_pdf`` in polaris script, which modifies the output name of the total scattering pdf

Bugfixes
############
- HRPD reduction :ref:`isis-powder-diffraction-hrpd-ref` now only subtracts empty run from the vanadium when it is subtracted from the sample run (i.e. when ``subtract_empty_instrument=True``) - note other ISIS instrument will always subtract an empty run from the vanadium.
- Remove the confusing "Replace input workspace" button in the :ref:`WANDPowderReduction <algm-WANDPowderReduction>` algorithm


Engineering Diffraction
-----------------------

New features
############
- Added functionality to call FindPeaksConvolve algorithm with good default values for ENGIN-X from the fitproperty browser in the Engineering UI :ref:`Fitting tab <ui engineering fitting>` resulting to add peaks found in the range specified with the sliders on the X axis.
- GSAS-II tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` now supports all lattice parameters being overridden (previously was assumed cubic)

Bugfixes
############
- Avoid overwriting Blank primary log setting in Engineering diffractions :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>` fitting tab :ref:`Fitting tab <ui engineering fitting>` when doing a sequential fit.
- Avoid loading non existing spectra in the :ref:`Engineering Diffraction interface <Engineering_Diffraction-ref>` :ref:`Fitting tab <ui engineering fitting>` when doing a fit following an ADS clear.
- GSAS-II tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` uses :ref:`LoadCIF <algm-LoadCIF>` to load phase files which fixes a bug reading the lattice parameters from .cif files with multiple loop blocks with elements with ``_atom*`` prefix.
- Fixed bug in GSAS-II tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` where lattice overridden in all phases provided (now only the first phase file lattice is overridden).
- Fixed bug in GSAS-II tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` where Pawley reflections from all phases were added to each individual phase.


Single Crystal Diffraction
--------------------------

New features
############
- Speedup :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` significantly by using ``scipy.signal.convolve`` instead of ``scipy.ndimag.convolve``.
- Remove ``RemoveOnEdge`` option in :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` as the algorithm uses new convolution method that ignores edges.
- New algorithm :ref:`FindGoniometerAngles <algm-FindGoniometerAngles>` that does a brute force search for the goniometer rotation angles that maximize the number of peaks indexed by the UB.
- New grouping options using ``HB3AAdjustSampleNorm`` for DEMAND data.
- Added Garnet to the menu items in the Diffraction interfaces list.
- Add method ``plot_integrated_peaks_MD`` to ``BaseSX`` to plot result of IntegratePeaksMD and save in pdf
- New algorithm :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` to integrate single-crystal Bragg peaks in a workspace with an x-unit of TOF adapted from an algorithm in SXD2001 by Gutmann, M. J. (2005)
- Added option to use :ref:`IntegratePeaks1DProfile <algm-IntegratePeaks1DProfile>` in ``BaseSX`` class (for use in WISH and SXD reduction).
- Add option to find peaks using the ratio of variance/ mean in :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` - this is a peak finding criterion used in DIALS software Winter, G., et al.  Acta Crystallographica Section D: Structural Biology 74.2 (2018): 85-97.
- :ref:`FindSXPeaksConvolve <algm-FindSXPeaksConvolve>` is the default peak finding algorithm in the SXD reduction class.
- New option to apply ``LorentzCorrection`` to ``ConvertHFIRSCDtoMDE`` for monochromatic single crystal diffraction with rotation about the vertical axis.

Bugfixes
############
- Fix bug in :ref:`LoadNexusProcessed <algm-LoadNexusProcessed>` where peak shapes are skipped
- Fix error when no peak found in vicinity of predicted position in ``IntegratePeaksShoeboxTOF``
- Fix bug in :ref:`IntegratePeaksShoeboxTOF <algm-IntegratePeaksShoeboxTOF>` where no shoebox dimensions could be retrieved from strong peaks unless an output file was specified.

:ref:`Release 6.10.0 <v6.10.0>`