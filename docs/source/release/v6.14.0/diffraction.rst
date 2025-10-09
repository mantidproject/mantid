===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

Powder Diffraction
------------------

New features
############
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can skip loading particular logs using the (mutually exclisive) ``LogAllowList`` and ``LogBlockList`` properties
- In Polaris reduction, output `merged_S_of_Q_minus_one` workspace before applying Fourier filter to the PDF. To differntiate this from resultant workspace after applying the Fourier filter and reverse transformed, the corresponding workspace was renamed from `*_merged_Q` to `*_merged_Q_r_FT`.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can now filter out bad pulses during loading.
- :ref:`PDConvertRealSpace <algm-PDConvertRealSpace>` and :ref:`PDConvertReciprocalSpace <algm-PDConvertReciprocalSpace>` will not get added to the algorithm factory if PyStoG is not installed. This package is now an optional dependency of Mantid. For more information on PyStoG, see https://pystog.readthedocs.io/en/latest/.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` can do event filtering with a Splitter Workspace but only with a single output workspace and filtering on pulse time only.
- Add parameter ``wavelength_lims`` to :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>`) to crop focussed data by wavelength before calculating the pair distribution function (pdf).
- Add parameter ``r_lims`` to :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>`) to specify min and max r-value in output pdf.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now allows different binning to be specified for each output spectra.
- :ref:`AlignAndFocusPowderSlim <algm-AlignAndFocusPowderSlim>` now allows different binning units, the default being d-spacing. The output will always be in time-of-flight.

Bugfixes
############
- Update zero-padding for GEM and ALF (ISIS instruments) so that users can load runs from ALF00090565 and GEM00097479 onwards.
- Fixes bug in :ref:`DiffractionFocussing <algm-DiffractionFocussing>` where NaN data values were introduced when explicit binning domains were requested that exceeded the actual input-data domains.  This bug only occurred when `PreserveEvents=False`.
- Update default TOF cropping values for focussed banks for ``mode="pdf"`` in POLARIS :ref:`Polaris diffraction <isis-powder-diffraction-polaris-ref>`)
- Fixed a potential crash when running :ref:`DiffractionFocussing <algm-DiffractionFocussing>` from the algorithm dialogue and setting ``InputWorkspace`` to a workspace group.
- The ``InputWorkspace`` property of :ref:`DiffractionFocussing <algm-DiffractionFocussing>` can no longer take workspaces in TOF (deprecated 29/04/21).


Engineering Diffraction
-----------------------

New features
############
- Add module of classes in ``Engineering.pawley_utils`` to perform Pawley refinements for focussed spectra and 2D Pawley refinements for POLDI (frame overlap diffractometer).
- Support batch refinement of multi-run datasets using a single instrument parameter file in :ref:`GSASII tab <ui engineering gsas>` tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` GUI.
- Removed support for mixed single-region and multi-instrument file configurations in :ref:`GSASII tab <ui engineering gsas>` tab of :ref:`Engineering Diffraction interface<Engineering_Diffraction-ref>` GUI.
- Texture Analysis can now be performed using the logic included in ``TextureUtils`` and a collection of scripts that can be found in ``diffraction/ENGINX/Texture`` within the :ref:`mantid script repository <WorkbenchScriptRepository>`.
- Focusing using the ``focus_run`` method in ``EnggUtils`` will now save a combined workspace with all detector groups' spectra, rather than just saving each individual spectra in a separate workspace.
- Performance improvements to :ref:`algm-PoldiAutoCorrelation-v6` and function to simulate POLDI 2D workspace.
- New parameter ``InterpolationMethod`` in :ref:`algm-PoldiAutoCorrelation-v6`. The default value ``Linear`` preserves existing behaviour (linear interpolation), an additonal option method ``Nearest`` has been added for faster execution.

Bugfixes
############



Single Crystal Diffraction
--------------------------

New features
############
- :ref:`LoadWANDSCD <algm-LoadWANDSCD>` will now load the sample environment logs
- Algorithm to find UB from 1 peak and defined scattering plane.
- New algorithm :ref:`FindUBFromScatteringPlane <algm-FindUBFromScatteringPlane>` to find UB Matrix given lattice parameters, scattering plane and 1 peak for a sample

Bugfixes
############


:ref:`Release 6.14.0 <v6.14.0>`
