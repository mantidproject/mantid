.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

**Run numbers**:
All run numbers provided will be reduced together as an aggregate sample.
Example:
2144-2147,2156 will reduce runs from (and including) 2144 up to
(and including) 2147, plus 2156.

**Flux Normalization Type**:
Normalize the number of counts by either: (1) counts recorded in the monitor,
(2) total proton charge, or (3) run duration. This flux normalization is
separately applied to the aggregate sample, aggregate vanadium, and
aggregated background.

**Mask file**: The default file (BASIS_Mask_default_diff.xml) masks all
inelastic detectors as well as the tips of the diffraction tubes. The file
is in directory /SNS/BSS/shared/autoreduce/new_masks_08_12_2015.

**OutputWorkspace**: a one-histogram :ref:`MatrixWorkspace <MatrixWorkspace>`
is produced containing the intensities versus momentum transfer `Q`. In addition, a
workspace of same name plus suffix `_angle` is produced. The latter workspace
contains intensities versus scattering angle. If background runs are
provided, these workspaces contain the intensities of the sample minus those
of the background.

**BackgroundRuns**: one or more run numbers to describe the background.
Background runs are reduced together.

**BackgroundScale**: background intensities are rescaled by this number
when subtracting background from sample.

**OutputBackground**: if populated, a one-histogram
:ref:`MatrixWorkspace <MatrixWorkspace>` is produced containing the
background intensities versus momentum transfer `Q`. In addition, a
workspace of same name plus suffix `_angle` is produced. The latter workspace
contains intensities versus scattering angle. Rescaling of the
background intensities is not applied when producing these workspaces.

**VanadiumRuns**: a set of runs to be reduced jointly provide an incoherent
and isotropic scattering to determine instrument efficiency per detector. If
no vanadium is provided, all detectors are assumed to have the same efficiency.
The vanadium intensity is integrated in each detector and used to normalize
both the sample and the background intensities.

Usage
-----

**Determine powder diffraction pattern:**

.. code-block:: python

    from mantid.simpleapi import BASISPowderDiffraction
    BASISPowderDiffraction(RunNumbers='74799',
                           OutputWorkspace='powder',
                           BackgroundRuns='75527',
                           OutputBackground='background'
                           VanadiumRuns='64642')
.. figure:: /images/BASISPowderDiffraction.png

The color image shows the nine diffraction detectors with a Bragg peak spilling
intensity on the edges of two tubes at a scattering angle of 60 degrees.

Developer's Corner
------------------

Adding the Previous Pulse
#########################

because of the legacy hardware used at BASIS (ROC2 instead of ROC5),
the diffraction detectors frame is forced to coincide with the
inelastic detectors frame, introducing a shift in the minimal TOF.

In the figures below, we have represented the TOF and wavelength dependence
of the intensities for the monitors (black), sample from the current
pulse (red), and sample from the previous pulse (green)

**111 Reflection:**

.. figure:: /images/BASISPowderDiffraction_2.png

**311 Reflection:**

.. figure:: /images/BASISPowderDiffraction_2.png

The figures show that the "slow" neutrons from the previous pulse should be
accounted as the fast neutrons for the current pulse.

The solution is to: (1) pile together events from the previous and current
pulses; (2) discard events with under-represented wavelegths. We use the
monitor counts for the last step.

.. categories::

.. sourcelink::


