.. _interface-inelastic-corrections:

Inelastic Corrections
=====================

.. contents:: Table of Contents
  :local:

Overview
--------

Provides correction routines for quasielastic, inelastic and diffraction
reductions.

These interfaces do not support GroupWorkspace's as input.

.. interface:: Corrections
  :align: right
  :width: 750

Action Buttons
~~~~~~~~~~~~~~

Settings
  Opens the :ref:`Settings <inelastic-interface-settings>` GUI which allows you to
  customize the settings for this interface.

?
  Opens this help page.

Py
  Exports a Python script which will replicate the processing done by the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

Container Subtraction
---------------------

The Container Subtraction Tab is used to remove the container's contribution to a run.

Once run the corrected output and container correction is shown in the preview plot. Note
that when this plot shows the result of a calculation the X axis is always in wavelength,
however when data is initially selected the X axis unit matches that of the sample workspace.

The input and container workspaces will be converted to wavelength (using
:ref:`ConvertUnits <algm-ConvertUnits>`) if they do not already have wavelength
as their X unit.

.. interface:: Corrections
  :widget: tabContainerSubtraction

Options
~~~~~~~

Sample
  Either a reduced file (_red.nxs) or workspace (_red), an :math:`S(Q,\omega)` file (_sqw.nxs) or workspace (_sqw), or an ELF file (_elf.nxs) or workspace (_elf) that represents the sample.

Container
  Either a reduced file (_red.nxs) or workspace (_red), an :math:`S(Q,\omega)` file (_sqw.nxs) or workspace (_sqw), or an ELF file (_elf.nxs) or workspace (_elf) that represents the container.

Scale Container by Factor
  Allows the container's intensity to be scaled by a given scale factor before being used in the corrections calculation.

Shift X-values by Adding
  Allows the X-values to be shifted by a specified amount.

Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Open Slice Viewer
  If enabled, it will open the slice viewer for the selected output workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save directory.

Calculate Paalman Pings
-----------------------

Calculates absorption corrections in the Paalman & Pings absorption factors that
could be applied to the data when given information about the sample (and
optionally the container) geometry.

.. interface:: Corrections
  :widget: tabCalculatePaalmanPings

Options
~~~~~~~

Sample Input
  A reduced file (*_red.nxs*) or workspace (*_red*).

Use Container
  If checked allows you to select a workspace for the container in a reduced file (*_red.nxs*) or workspace (*_red*).

Corrections Details
  These options will be automatically preset to the default values read from the sample workspace,
  whenever possible. They can be overridden manually.(see below)

Sample Shape
  Sets the shape of the sample, this affects the options for the shape details
  (see below).

Sample Details Method
  Choose to use a Chemical Formula or Cross Sections to set the neutron information in the sample using
  the :ref:`SetSampleMaterial <algm-SetSampleMaterial>` algorithm.

Sample/Container Mass density, Atom Number Density or Formula Number Density
  Density of the sample or container. This is used in the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm. If Atom Number Density is used, the NumberDensityUnit property is set to *Atoms* and if
  Formula Number Density is used then NumberDensityUnit is set to *Formula Units*.

Sample/Container Chemical Formula
  Chemical formula of the sample or container material. This must be provided in the
  format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm.

Cross Sections
  Selecting the Cross Sections option in the Sample Details combobox will allow you to enter coherent,
  incoherent and attenuation cross sections for the Sample and Container (units in barns).

Run
  Runs the processing configured on the current tab.

Plot Wavelength
  If enabled, it will plot a wavelength spectrum represented by the selected workspace indices.

Plot Angle
  If enabled, it will plot an angle bin represented by the neighbouring bin indices.

Save Result
  Saves the result in the default save directory.

Correction Details
~~~~~~~~~~~~~~~~~~

Emode
  The energy transfer mode. All the options except *Efixed* require the input workspaces to be in wavelength.
  In *Efixed* mode, correction will be computed only for a single wavelength point defined by ` Efixed` value.
  All the options except *Elastic* require the Efixed value to be set correctly.
  For flat plate, all the options except *Efixed*, are equivalent.
  In brief, use *Indirect* for QENS, *Efixed* for FWS and diffraction.
  *Efixed* can be used for QENS also, if the energy transfer can be neglected compared to the incident energy.
  See :ref:`CylinderPaalmanPingsCorrections <algm-CylinderPaalmanPingsCorrection>` for the details.

Efixed
  The value of the incident (indirect) or final (direct) energy in `mev`. Specified in the instrument parameter file.

Number Wavelengths
  Number of wavelength points to compute the corrections for. Ignored for *Efixed*.

Interpolate
  Whether or not to interpolate the corrections as a function of wavelength. Ignored for *Efixed*.

Shape Details
~~~~~~~~~~~~~

Depending on the shape of the sample different parameters for the sample
dimension are required and are detailed below.

Flat Plate
##########

.. interface:: Corrections
  :widget: pgFlatPlate

The calculation for a flat plate geometry is performed by the
:ref:`FlatPlatePaalmanPingsCorrection <algm-FlatPlatePaalmanPingsCorrection>`
algorithm.

Sample Thickness
  Thickness of sample in :math:`cm`.

Sample Angle
  Angle of the sample to the beam in degrees.

Container Front Thickness
  Thickness of front container in :math:`cm`.

Container Back Thickness
  Thickness of back container in :math:`cm`.

Cylinder
########

.. interface:: Corrections
  :widget: pgCylinder

The calculation for a cylindrical geometry is performed by the
:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
algorithm.

Sample Inner Radius
  Radius of the inner wall of the sample in :math:`cm`.

Sample Outer Radius
  Radius of the outer wall of the sample in :math:`cm`.

Container Outer Radius
  Radius of outer wall of the container in :math:`cm`.

Beam Height
  Height of incident beam :math:`cm`.

Beam Width
  Width of incident beam in :math:`cm`.

Step Size
  Step size used in calculation in :math:`cm`.

Annulus
#######

.. interface:: Corrections
  :widget: pgAnnulus

The calculation for an annular geometry is performed by the
:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
algorithm.

The options here are the same as for Cylinder.

Background
~~~~~~~~~~

The main correction to be applied to neutron scattering data is that for
absorption both in the sample and its container, when present. For flat plate
geometry, the corrections can be analytical and have been discussed for example
by Carlile [1]. The situation for cylindrical geometry is more complex and
requires numerical integration. These techniques are well known and used in
liquid and amorphous diffraction, and are described in the ATLAS manual [2].

The absorption corrections use the formulism of Paalman and Pings [3] and
involve the attenuation factors :math:`A_{i,j}` where :math:`i` refers to
scattering and :math:`j` attenuation. For example, :math:`A_{s,sc}` is the
attenuation factor for scattering in the sample and attenuation in the sample
plus container. If the scattering cross sections for sample and container are
:math:`\Sigma_{s}` and :math:`\Sigma_{c}` respectively, then the measured
scattering from the empty container is :math:`I_{c} = \Sigma_{c}A_{c,c}` and
that from the sample plus container is :math:`I_{sc} = \Sigma_{s}A_{s,sc} +
\Sigma_{c}A_{c,sc}`, thus :math:`\Sigma_{s} = (I_{sc} - I_{c}A_{c,sc}/A_{c,c}) /
A_{s,sc}`.

References:

1. C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)
2. A K Soper, W S Howells & A C Hannon, `RAL Report RAL-89-046 (1989) <http://wwwisis2.isis.rl.ac.uk/Disordered/Manuals/ATLAS/ATLAS%20manual%20v1.0%20Intro.pdf>`_
3. H H Paalman & C J Pings, `J Appl Phys 33 2635 (1962) <http://dx.doi.org/10.1063/1.1729034>`_

Calculate Monte Carlo Absorption
--------------------------------

The Calculate Monte Carlo Absorption tab provides a cross platform alternative to the
Calculate Paalman Pings tab. In this tab a Monte Carlo implementation is used to calculate the
absorption corrections.

.. interface:: Corrections
  :widget: tabAbsorptionCorrections

Options
~~~~~~~

Workspace Input
  A reduced file (*_red.nxs*) or workspace (*_red*).

Number Wavelengths
  The number of wavelength points for which a simulation is attempted.

Events
  The number of neutron events to generate per simulated point.

Interpolation
  Method of interpolation used to compute unsimulated values.

Maximum Scatter Point Attempts
  Maximum number of tries made to generate a scattering point within the sample (+ optional
  container etc). Objects with holes in them, e.g. a thin annulus can cause problems if this
  number is too low. If a scattering point cannot be generated by increasing this value then
  there is most likely a problem with the sample geometry.

Sparse Instrument
  Whether to spatially approximate the instrument for faster calculation.

Number Of Detector Rows
  Number of detector rows in the detector grid of the sparse instrument.

Number Of Detector Columns
  Number of detector columns in the detector grid of the sparse instrument.

Beam Height
  The height of the beam in :math:`cm`.

Beam Width
  The width of the beam in :math:`cm`.

Shape Details
  Select the shape of the sample (see specific geometry options below). Alternatively, select 'Preset' to use the Sample and Container geometries defined on the input workspace.

Use Container
  If checked, allows you to input container geometries for use in the absorption corrections.

Sample Details Method
  Choose to use a Chemical Formula or Cross Sections to set the neutron information in the sample using
  the :ref:`SetSampleMaterial <algm-SetSampleMaterial>` algorithm.

Sample/Container Mass density, Atom Number Density or Formula Number Density
  Density of the sample or container. This is used in the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm. If Atom Number Density is used, the NumberDensityUnit property is set to *Atoms* and if
  Formula Number Density is used then NumberDensityUnit is set to *Formula Units*.

Sample/Container Chemical Formula
  Chemical formula of the sample or container material. This must be provided in the
  format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm.

Cross Sections
  Selecting the Cross Sections option in the Sample Details combobox will allow you to enter coherent,
  incoherent and attenuation cross sections for the Sample and Container (units in barns).

Run
  Runs the processing configured on the current tab.

Plot Wavelength
  If enabled, it will plot a wavelength spectrum represented by the selected workspace indices.

Plot Angle
  If enabled, it will plot an angle bin represented by the neighbouring bin indices.

Save Result
  Saves the result in the default save directory.

Shape Details
~~~~~~~~~~~~~

Depending on the shape of the sample different parameters for the sample
dimension are required and are detailed below.

Preset
######

This option will use the Sample and Container geometries as defined in the input workspace. No further geometry inputs will be taken, though the Sample material can still be overridden.

Flat Plate
##########

.. interface:: Corrections
  :widget: pgAbsCorFlatPlate

Flat plate calculations are provided by the
:ref:`IndirectFlatPlateAbsorption <algm-IndirectFlatPlateAbsorption>` algorithm.

Sample Width
  Width of the sample in :math:`cm`.

Sample Height
  Height of the sample in :math:`cm`.

Sample Thickness
  Thickness of the sample in :math:`cm`.

Sample Angle
  Angle of the sample to the beam in degrees.

Container Front Thickness
  Thickness of the front of the container in :math:`cm`.

Container Back Thickness
  Thickness of the back of the container in :math:`cm`.

Annulus
#######

.. interface:: Corrections
  :widget: pgAbsCorAnnulus

Annulus calculations are provided by the :ref:`IndirectAnnulusAbsorption
<algm-IndirectAnnulusAbsorption>` algorithm.

Sample Inner Radius
  Radius of the inner wall of the sample in :math:`cm`.

Sample Outer Radius
  Radius of the outer wall of the sample in :math:`cm`.

Container Inner Radius
  Radius of the inner wall of the container in :math:`cm`.

Container Outer Radius
  Radius of the outer wall of the container in :math:`cm`.

Sample Height
  Height of the sample in :math:`cm`.

Cylinder
########

.. interface:: Corrections
  :widget: pgAbsCorCylinder

Cylinder calculations are provided by the
:ref:`IndirectCylinderAbsorption <algm-IndirectCylinderAbsorption>` algorithm.

Sample Radius
  Radius of the outer wall of the sample in :math:`cm`.

Container Radius
  Radius of the outer wall of the container in :math:`cm`.

Sample Height
  Height of the sample in :math:`cm`.

.. _apply_absorp_correct:

Apply Absorption Corrections
----------------------------

The Apply Corrections tab applies the corrections calculated in the Calculate Paalman
Pings or Calculate Monte Carlo Absorption tabs of the Inelastic Data Corrections interface.

This uses the :ref:`ApplyPaalmanPingsCorrection
<algm-ApplyPaalmanPingsCorrection>` algorithm to apply absorption corrections in
the form of the Paalman & Pings correction factors. When *Use Container* is disabled
only the :math:`A_{s,s}` factor must be provided, when using a container the
additional factors must be provided: :math:`A_{c,sc}`, :math:`A_{s,sc}` and
:math:`A_{c,c}`.

Once run the corrected output and container correction is shown in the preview plot. Note
that when this plot shows the result of a calculation the X axis is always in
wavelength, however when data is initially selected the X axis unit matches that
of the sample workspace.

The input and container workspaces will be converted to wavelength (using
:ref:`ConvertUnits <algm-ConvertUnits>`) if they do not already have wavelength
as their X unit.

The binning of the sample, container and corrections factor workspace must all
match, if the sample and container do not match you will be given the option to
rebin (using :ref:`RebinToWorkspace <algm-RebinToWorkspace>`) the sample to
match the container, if the correction factors do not match you will be given
the option to interpolate (:ref:`SplineInterpolation
<algm-SplineInterpolation>`) the correction factor to match the sample.

.. interface:: Corrections
  :widget: tabApplyAbsorptionCorrections

Options
~~~~~~~

Sample
  A reduced file (*_red.nxs*) or workspace (*_red*).

Corrections
  The calculated corrections workspace produced from one of the preview two tabs.

Geometry
  Sets the sample geometry (this must match the sample shape used when calculating
  the corrections).

Use Container
  If checked allows you to select a workspace for the container in the format of
  either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Scale Container by factor
  Allows the container intensity to be scaled by a given scale factor before
  being used in the corrections calculation.

Shift X-values by Adding
  Allows the X-values of the container to be shifted by a specified amount.

Rebin Container to Sample
  Rebins the container to the sample.

Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Open Slice Viewer
  If enabled, it will open the slice viewer for the selected output workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save directory.

.. categories:: Interfaces Inelastic
