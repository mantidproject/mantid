===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can skip loading particular logs using the (mutually exclisive) ``LogAllowList`` and ``LogBlockList`` properties.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can now filter out bad pulses during loading by setting the ``FilterBadPulses`` to ``True`` (cutoff can be adjusted with ``BadPulsesLowerCutoff``).
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now allows different binning to be specified for each output spectra by setting ``XMin`` or ``XMax`` to a list of values.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now allows different input binning units by setting the ``BinningUnits`` property to one of ``"dSpacing"``, ``"TOF"`` or ``"MomentumTransfer"``, the default being d-spacing. The output will always be in time-of-flight.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can do event filtering with a Splitter Workspace when the ``SplitterWorkspace`` property is set. This can be done only with a single output workspace and when filtering on pulse time.
- :ref:`PDConvertRealSpace <algm-PDConvertRealSpace>` and :ref:`PDConvertReciprocalSpace <algm-PDConvertReciprocalSpace>` will not get added to the algorithm factory if PyStoG is not installed. This package is now an optional dependency of Mantid. For more information on PyStoG, see `the PyStogG docs <https://pystog.readthedocs.io/en/latest/>`_.
- In Polaris reduction, ``merged_S_of_Q_minus_one`` workspace is output before applying Fourier filter to the PDF. To differntiate this from the resultant workspace after applying the Fourier filter and reverse transformed, the corresponding workspace was renamed from ``*_merged_Q`` to ``*_merged_Q_r_FT``.
- Added parameter ``wavelength_lims`` to :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>` to crop focussed data by wavelength before calculating the pair distribution function (PDF).
- Added parameter ``r_lims`` to :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>` to specify min and max r-value in output PDF.
- Updated default TOF cropping values for focussed banks for ``mode="pdf"`` in POLARIS :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>`
- :ref:`SNSPowderReduction<algm-SNSPowderReduction>` has been updated to more correctly apply absorption corrections.

Bugfixes
############
- Updated zero-padding for GEM and ALF (ISIS instruments) so that users can load runs from ALF00090565 and GEM00097479 onwards.
- A bug in :ref:`DiffractionFocussing <algm-DiffractionFocussing>` where ``NaN`` data values were introduced when explicit binning domains were requested that exceeded the actual input-data domains has been fixed.  This bug only occurred when ``PreserveEvents=False``.
- Fixed a potential crash when running :ref:`DiffractionFocussing <algm-DiffractionFocussing>` from the algorithm dialogue and setting ``InputWorkspace`` to a workspace group.
- The ``InputWorkspace`` property of :ref:`DiffractionFocussing <algm-DiffractionFocussing>` can no longer take workspaces in TOF (deprecated 29/04/21).


Engineering Diffraction
-----------------------

New features
############
- Add module of classes in ``Engineering.pawley_utils`` to perform Pawley refinements for focussed spectra and 2D Pawley refinements for POLDI (frame overlap diffractometer).
- Support batch refinement for multiple focussed (.gss) files using a single instrument group (.prm) file in the :ref:`GSASII tab <ui engineering gsas>` of the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` GUI.  Note the .prm file should have the same number of groups as the number of spectra in an individual .gss file.
- Removed support for specifying multiple focussed .gss files (e.g. one for each bank in ENGINX) for a single instrument group (.prm) file in :ref:`GSASII tab <ui engineering gsas>` of the :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` GUI.
- Texture Analysis can now be performed using the logic included in ``Engineering.texture.TextureUtils`` and a collection of scripts that can be found in ``diffraction/ENGINX/Texture`` within the :ref:`mantid script repository <WorkbenchScriptRepository>`.
- Focusing using the ``focus_run`` method in ``Engineering.EnggUtils`` will now save a combined workspace with all detector groups' spectra, rather than saving each spectra in a separate workspace.
- Performance improvements have been made to :ref:`algm-PoldiAutoCorrelation-v6` and a function to simulate POLDI 2D workspace.
- New property ``InterpolationMethod`` added to :ref:`algm-PoldiAutoCorrelation-v6`. The default value ``"Linear"`` preserves existing behaviour (linear interpolation), and ``"Nearest"`` can be used for faster execution.

Bugfixes
############



Single Crystal Diffraction
--------------------------

New features
############
- :ref:`LoadWANDSCD <algm-LoadWANDSCD>` will now load the sample environment logs.
- New algorithm :ref:`FindUBFromScatteringPlane <algm-FindUBFromScatteringPlane>` to find UB Matrix given lattice parameters, scattering plane and 1 peak for a sample.
- Add ``UpdateUB`` option to :ref:`algm-IndexPeaks` that saves the optimized UB matrix in the case where there is a single run and ``CommonUBForAll=False``.

Bugfixes
############


:ref:`Release 6.14.0 <v6.14.0>`
