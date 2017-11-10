Indirect Corrections
====================

.. contents:: Table of Contents
  :local:

Overview
--------

.. interface:: Corrections
  :align: right
  :width: 350

Provides correction routines for quasielastic, inelastic and diffraction
reductions.

These interfaces do not support GroupWorkspace as input.

Action Buttons
~~~~~~~~~~~~~~

?
  Opens this help page.

Py
  Exports a Python script which will replicate the processing done by the current tab.

Run
  Runs the processing configured on the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

Calculate Paalman Pings
-----------------------

.. interface:: Corrections
  :widget: tabCalculatePaalmanPings

Calculates absorption corrections in the Paalman & Pings absorption factors that
could be applied to the data when given information about the sample (and
optionally can) geometry.

Options
~~~~~~~

Input
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Use Can
  If checked allows you to select a workspace for the container in the format of
  either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Sample Shape
  Sets the shape of the sample, this affects the options for the shape details
  (see below).

Sample/Can Number Density
  Density of the sample or container.

Sample/Can Chemical Formula
  Chemical formula of the sample or can material. This must be provided in the
  format expected by the :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  algorithm.

Plot Output
  Plots the :math:`A_{s,s}`, :math:`A_{s,sc}`, :math:`A_{c,sc}` and
  :math:`A_{c,c}` workspaces as spectra plots.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Correction Details
~~~~~~~~~~~~~~~~~~

These options will be automatically preset to the default values read from the sample workspace, whenever possible.
They can be overridden manually.

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

NumberWavelength
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
  Sample angle in degrees.

Can Front Thickness
  Thickness of front container in :math:`cm`.

Can Back Thickness
  Thickness of back container in :math:`cm`.

Cylinder
########

.. interface:: Corrections
  :widget: pgCylinder

The calculation for a cylindrical geometry is performed by the
:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
algorithm, this algorithm is currently only available on Windows as it uses
FORTRAN code dependant of F2Py.

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
  Step size used in calculation.

Annulus
#######

.. interface:: Corrections
  :widget: pgAnnulus

The calculation for an annular geometry is performed by the
:ref:`CylinderPaalmanPingsCorrection <algm-CylinderPaalmanPingsCorrection>`
algorithm, this algorithm is currently only available on Windows as it uses
FORTRAN code dependant of F2Py.

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

Apply Paalman Pings
-------------------

.. interface:: Corrections
  :widget: tabApplyPaalmanPings

The Apply Corrections tab applies the corrections calculated in the Calculate
Corrections tab of the Indirect Data Analysis interface.

This uses the :ref:`ApplyPaalmanPingsCorrection
<algm-ApplyPaalmanPingsCorrection>` algorithm to apply absorption corrections in
the form of the Paalman & Pings correction factors. When *Use Can* is disabled
only the :math:`A_{s,s}` factor must be provided, when using a container the
additional factors must be provided: :math:`A_{c,sc}`, :math:`A_{s,sc}` and
:math:`A_{c,c}`.

Once run the corrected output and can correction is shown in the preview plot,
the Spectrum spin box can be used to scroll through each spectrum. Note that
when this plot shows the result of a calculation the X axis is always in
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

Options
~~~~~~~

Input
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Geometry
  Sets the sample geometry (this must match the sample shape used when running
  Calculate Corrections).

Use Can
  If checked allows you to select a workspace for the container in the format of
  either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Scale Can by factor
  Allows the container intensity to be scaled by a given scale factor before
  being used in the corrections calculation.

Use Corrections
  The Paalman & Pings correction factors to use in the calculation, note that
  the file or workspace name must end in either *_flt_abs* or *_cyl_abs* for the
  flat plate and cylinder geometries respectively.

Plot Output
  Gives the option to create either a spectra or contour plot (or both) of the
  corrected workspace.

Save Result
  If enabled the result will be saved as a NeXus file in the default save
  directory.

Absorption Corrections
----------------------

.. interface:: Corrections
  :widget: tabAbsorptionCorrections

The Absorption Corrections tab provides a cross platform alternative to the
previous Calculate and Apply Corrections tabs.

Common Options
~~~~~~~~~~~~~~

Sample Input
  Used to select the sample from either a file or a workspace already loaded
  into Mantid.

Use Container
  Used to enable or disable use of a container and selects one from either a
  file or loaded workspace.

Shape
  Select the shape of the sample (see specific geometry options below).

Number Wavelengths
  Number of wavelengths for calculation

Events
  Number of neutron events

Mass Density/Number Density
  Mass density or Number Density for either the sample or container.

Chemical Formula
  Chemical formula for either the sample or container in the format expected by
  :ref:`SetSampleMaterial <algm-SetSampleMaterial>`.

Use Container Corrections
  Enables full container corrections, if disabled only a can subtraction will be
  performed.

Scale
  Scale factor to scale container input by.

Keep Correction Factors
  If checked a :ref:`WorkspaceGroup` containing the correction factors will also
  be created, this will have the suffix *_Factors*.

Plot Result
  If clicked the corrected workspace and correction factors will be plotted.

Save Result
  If Clicked the corrected workspace and (if *Keep Correction Factors* is
  checked) the correction factor workspace will be saved as a NeXus file in the
  default save directory.

Flat Plate
~~~~~~~~~~

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
  Angle of the sample to the beam in radians.

Container Front Thickness
  Thickness of the front of the container in :math:`cm`.

Container Back Thickness
  Thickness of the back of the container in :math:`cm`.

Annulus
~~~~~~~

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
~~~~~~~~

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


Container Subtraction
---------------------

.. interface:: Corrections
  :widget: tabContainerSubtraction
  
The Container Subtraction Tab is used to remove the containers contribution to a run.

Once run the corrected output and can correction is shown in the preview plot, the Spectrum 
spin box can be used to scroll through each spectrum. Note that when this plot shows the 
result of a calculation the X axis is always in wavelength, however when data is initially 
selected the X axis unit matches that of the sample workspace.

The input and container workspaces will be converted to wavelength (using
:ref:`ConvertUnits <algm-ConvertUnits>`) if they do not already have wavelength
as their X unit.
 
Options
~~~~~~~

Input Sample
  Either a reduced file (_red.nxs) or workspace (_red) or an S(Q,\omega) file (_sqw.nxs) or workspace (_sqw) that represents the sample.
  
Input Container
  Either a reduced file (_red.nxs) or workspace (_red) or an S(Q,\omega) file (_sqw.nxs) or workspace (_sqw) that represents the container.
  
Scale Can by Factor
  Allows the container intensity to be scaled by a given scale factor before being used in the corrections calculation.

Plot Output
  Gives the option to create either a spectra or contour plot (or both) of the corrected workspace.
  
Save Result
  If enabled the result will be saved as a NeXus file in the default save directory.

.. categories:: Interfaces Indirect
