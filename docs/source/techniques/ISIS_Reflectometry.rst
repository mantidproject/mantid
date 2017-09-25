.. _ISIS Reflectometry:

====================
 ISIS Reflectometry
====================

.. contents::
  :local:




Overview
========

This document explains set up of reflectometry instruments at ISIS and how Mantid is used to perform reduction of ISIS reflectometry data.

Introduction
------------
Reflectometry is a non-invasive technique that allows us to analyse the properties of thin films. The technique is based on the reflection of neutrons at the interfaces of interest; this is analogous to how light (travelling through air) reflects off the surface of water (our interface of interest). Analysis of the reflected signal can give us insight into the thin films' properties, and the interfaces between them. A highly collimated beam of neutrons illuminates a flat surface and the reflected intensity is measured as a function of angle and neutron wavelength. The reflectivity profile can be fitted to a model after the experiment to provide detailed information about the structure, including the thickness, density and roughness of any thin films layered on the surface.


For more information, see the `ISIS website <https://www.isis.stfc.ac.uk/Pages/Reflectometry.aspx>`__.

Experimental Setup
------------------

The main components in the instrument are shown below:

.. figure:: /images/ISISReflectometry_setup_diagram.png
   :align: center
   :alt: Diagram showing the basic setup of ISIS Reflectometry instruments

   Basic reflectometry instrument setup at ISIS. The black line is the incident beam and the red line the reflected beam. The dotted black line is the horizon of the sample which is at an angle :math:`\theta_i` with respect to the incident beam.


* *Source*: The source of neutrons
* *Sample*: The material under analysis
* *Slits*: These are used to collimate the beam to only illuminate the required area (the sample). The slits define the *resolution* in momentum transfer, :math:`Q`.
* *Monitors*: These are used to normalize the detector signal
* *Detectors*: Scattered neutrons are recorded in banks of detectors located at different *scattering angles*, :math:`2\theta_{det}`. Note that the terminology here differs from that in neutron diffraction, where :math:`\theta` is referred to as the scattering angle rather than :math:`2\theta`.
* :math:`\theta_i`: the *incident angle*, that is, the angle between the incident beam and the sample
* :math:`\theta_f`: the *final angle*, that is, the angle between the reflected beam and the sample. In specular reflection, :math:`\theta_i = \theta_f`. In off-specular analysis, :math:`\theta_i \neq \theta_f`.

In addition, some instruments also use:

* *Polarizers*: These use neutron spin dependent supermirrors to separate out the two possible neutron spin states: *up* and *down*, which align parallel or anti-parallel to an applied magnetic field.
* *Supermirrors*: Some samples, such as liquids, cannot be angled. Therefore "supermirrors" can be used to change the incident angle of the beam to enable multiple angles to be measured from the surface.

Reduction
=========

The main reduction algorithm is :ref:`algm-ReflectometryReductionOneAuto`, which can be used from a python script or via the :ref:`ISIS Reflectometry Interface <interface-isis-refl>`. It currently only deals with specular reflection, but off-specular analysis is planned. It also currently only deals with point detectors or linear detectors, but 2D detectors are planned.

Specular Reflection
-------------------

In an experiment, the reflected intensity, :math:`I` (i.e. the number of neutrons received at the detector) is measured as a function of time-of-flight, :math:`TOF`. The desired outcome of the reduction is a one-dimensional plot (summed over the detectors) of reflectivity normalised by an un-reflected beam, :math:`I_0` against momentum transfer, :math:`Q`.

To achieve this, the input workspace in :math:`TOF` is first converted to wavelength, :math:`\lambda`, and normalised by monitors; direct beam and transmission runs are optionally applied. The workspace is summed over all detectors in the region of interest, using either constant-:math:`\lambda` or constant-:math:`Q` binning. The result is a one-dimensional array of :math:`I` against :math:`\lambda`. The summed workspace is converted to :math:`Q` and rebinned to the required resolution, as determined either by the slits or the user. The :ref:`algm-NRCalculateSlitResolution` algorithm is used to calculate the slit resolution.

The resulting one-dimensional plot of :math:`I` against :math:`Q` is typically referred to as ``IvsQ``. :ref:`algm-ReflectometryReductionOneAuto` also outputs the unbinned workspace in :math:`Q`, as well as the summed workspace in :math:`\lambda`, and these are typically referred to as ``IvsQ_unbinned`` and ``IvsLam`` respectively.

An experiment may be repeated with different :math:`\theta_i` and the results stitched together in order to obtain the reflectivity over a greater range of :math:`Q`.

A sample under certain conditions (temperature, magnetic field, etc) is usually measured at two or three different incident angles, :math:`\theta_i`. This means that we typically end up with two or three :math:`TOF` workspaces that are combined (*stitched*) and processed to give a single plot covering a larger range of :math:`Q`.

The actual reduction is relatively simple and produces a simple one dimensional plot which can be saved as an ASCII file. However, there can be many files to deal with and we need to make sure that we process the correct runs together and with the correct parameters, transmission runs etc.

Divergent Beam
##############

If the divergence of the input beam is significant, we need to take the variability in :math:`\theta_i` and :math:`\theta_f` into account. Consider a range of incident angles on a sample of fixed orientation:

.. figure:: /images/ISISReflectometry_divergent_beam_diagram.png
   :align: center
   :alt: Diagram showing the divergent beam case

   A range of incident angles from a divergent beam on a sample of fixed orientation :math:`\theta_i`. The black lines show the expected beam and reflection directions, and the red lines show a divergent beam path and reflection.

The difference between the actual and expected beam directions is the same as that between the actual and expected reflection directions:

:math:`\phi = 2\theta_{det} - 2\theta_i`

Therefore, :math:`\theta_f` can be calculated as:

:math:`\theta_f = \theta_i + \phi = 2\theta_{det} - \theta_i`

The effect of the divergence is that the data follow lines of constant-:math:`Q`. Therefore we need to sum along lines of constant-:math:`Q` rather than constant-:math:`\lambda`. :ref:`algm-ReflectometryReductionOneAuto` has an option to do this.

.. figure:: /images/ISISReflectometry_divergent_beam_measured.png
   :align: center
   :alt: Plot showing the measured intensity

   Measured intensity for each detector vs :math:`TOF`. The data follow lines of constant :math:`Q`.

.. figure:: /images/ISISReflectometry_divergent_beam_result.png
   :align: center
   :alt: Plot showing the reduced data

   The reduced data as a plot of :math:`I` vs :math:`Q`, showing the improved resolution when summing in :math:`Q` rather than :math:`\lambda`

Non-Flat Sample
###############

A bent sample causes variability in the reflected angle, :math:`\theta_f`. It can be considered as a flat sample of variable orientation:

.. figure:: /images/ISISReflectometry_non_flat_sample_diagram.png
   :align: center
   :alt: Diagram showing the non-flat sample case

   Divergence in the reflected angle from a non-flat sample is considered as a flat sample of variable orientation. The black lines show the incident beam and the expected reflection direction with the sample horizon at :math:`\theta_i`. The red line shows a divergent reflected beam, and the dotted blue line shows the related sample horizon.

The difference between the actual and expected reflection directions is:

:math:`\phi = 2\theta_{det} - 2\theta_i`

Therefore, :math:`\theta_f` can be calculated as:

:math:`\theta_f = \theta_i + \frac{\phi}{2} = \frac{2\theta_{det}}{2}`

Similarly to the divergent beam case, the data should be summed along lines of constant-:math:`Q` using the relevant options in :ref:`algm-ReflectometryReductionOneAuto`.

Instruments
===========

ISIS Instruments
----------------

There are five reflectometry instruments at ISIS:

* *Inter*: High-intensity reflectometer. Specialised for free liquid surfaces.
* *Offspec*: Polarised neutron reflectometer with optional polarisation analysis, using a high resolution position sensitive detector. Used to study magnetic ordering in and between the layers and surfaces of thin film materials.
* *Polref*: Polarised neutron reflectometer. Used to study magnetic ordering in and between the layers and surfaces of thin film materials.
* *Crisp*: Designed for studies of a wide range of interfacial phenomena.
* *Surf*: Optimised for higher flux and short wavelengths. Designed for liquid interface research.

Detectors
---------
Currently at ISIS we deal with two types of detector: point-detectors (e.g. Inter) or multi-/linear-detectors (e.g. Polref and Offspec). Note that most instruments have both point and linear detectors. We are expecting to add 2D detectors in the near future.

Because runs are performed at different incident angles, the **detectors are moved** between different runs. Some instruments (e.g. Inter) move detectors vertically, whereas others (Polref, Offspec) rotate them around the sample.

Historically, detector positions needed to be adjusted within the reflectometry reduction algorithms. The :ref:`algm-SpecularReflectionPositionCorrect` algorithm deals with this. However, some instruments (e.g. Inter) now move detectors to the correct position on load, so correcting positions within the reflectometry algorithms is not required. This is the preferred approach going forward.

Instrument Definition Files
---------------------------

Mantid can handle instruments with different **reference frames** because it uses the beam direction, sample position, detector positions, etc. The reference frames currently used by ISIS reflectometry instruments are:

* Inter, Offspec, Crisp and Surf define the beam direction along the :math:`z` axis and *Up* (perpendicular to the beam) along the :math:`y` axis.
* Polref defines the beam direction along the :math:`x` axis and *Up* along the :math:`z` axis.

The way in which **components are arranged** in the IDF is different. Some instruments, such as Offspec, have a component “DetectorBench” that is the parent component of all the detectors. Others don’t have this component. This has to be taken into account when moving detectors.

Some of the instrument IDFs are set up such that detectors are at the correct **position on loading** a run. Some instruments are not be set up to do this yet, so :ref:`algm-ReflectometryReductionOneAuto` has an option to correct detector positions using another algorithm, :ref:`algm-SpecularReflectionPositionCorrect`. It is important that the detectors are in the correct position in order for Mantid algorithms to produce the correct results, otherwise some calculations (e.g. the conversion from :math:`\lambda` to :math:`Q`) will be wrong.


Mantid
======

Algorithms
----------

The main reduction algorithm is :ref:`algm-ReflectometryReductionOneAuto`. This sets a lot of the input properties from defaults in the instrument parameter file. It must also populate some input properties so that they can be updated in the GUI (this has to work both for single period datasets and multi period datasets). This algorithm is a wrapper around :ref:`algm-ReflectometryReductionOne`, which actually does the work. This arrangement seems to be unusual in Mantid.

:ref:`algm-SpecularReflectionPositionCorrect` can be used to correct detector positions if they are not at the correct position when loaded. It can shift them vertically or rotate then around the sample position. This algorithm is called as a child by :ref:`algm-ReflectometryReductionOneAuto`.

Related to :ref:`algm-ReflectometryReductionOne` and :ref:`algm-ReflectometryReductionOneAuto` we also have :ref:`algm-CreateTransmissionWorkspace` and :ref:`algm-CreateTransmissionWorkspaceAuto`, which convert transmission run(s) to wavelength and stitches transmission runs together when two are provided.

:ref:`algm-Stitch1DMany` does the work to stitch multiple runs together, which is quite a complicated operation.

:ref:`algm-ConvertToReflectometryQ`: This algorithm is generally used to examine off-specular scattering. The input is a workspace in wavelength, and the output is a :math:`QxQz` map (or :math:`KiKf` or :math:`PiPf`). It doesn't normalize by monitors, transmission run, etc (in fact, scientists typically run :ref:`algm-ReflectometryReductionOneAuto` prior to running this algorithm, so that they obtain the normalized intensity).

Interface
---------
The :ref:`ISIS Reflectometry Interface <interface-isis-refl>` provides a graphical front-end for the :ref:`algm-ReflectometryReductionOneAuto` algorithm. It includes the facility to:

* batch process all runs from an experiment (or a selected subset);
* apply default settings, which can be overridden on a per-run basis;
* process data in histogram or event modes;
* output processing steps to an IPython notebook; and
* output reduced data to ASCII files.

See the :ref:`full documentation <interface-isis-refl>` for more information.

Note that the main table on the ``Runs`` tab is designed to be a :ref:`generic batch-processing table <DataProcessorWidget_DevelopersGuide-ref>` which can be customised and re-used for other technique areas in their own interfaces. The table and interface are both tested in unit tests using gmock. The interface uses the MVP pattern at different levels and communication happens between presenters.

Note that the current interface replaces the ``ISIS Reflectometry (Old)`` interface, which was written in Python and had several limitations, including lack of automated testing. The old interface will shortly be removed.

.. categories:: Techniques
