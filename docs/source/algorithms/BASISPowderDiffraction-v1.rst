.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

**Run numbers**:
All run numbers provided will be reduced together.
Example:
2144-2147,2156 will reduce runs from (and including) 2144 up to
(and including) 2147, plus 2156.

**Mask file**: The default file (BASIS_Mask_default_diff.xml) masks all
inelastic detectors as well as the tips of the diffraction tubes. The file
is in directory /SNS/BSS/shared/autoreduce/new_masks_08_12_2015.

**OutputWorkspace**: a one-histogram :ref:`MatrixWorkspace2D <MatrixWorkspace2D>`
is produced containing the intensities versus momentum transfer `Q`. In addition, a
workspace of same name plus suffix `_angle` is produced. The latter workspace
contains intensities versus scattering angle. If background runs are
provided, these workspaces contain the intensities of the sample minus those
of the background.

**MonitorNormalization**: intensities will be divided by the integrated
intensity collected at the monitor. Integration of the histogram of monitor
intensities versus wavelength over a wavelength range featuring significant
neutron counts.

**BackgroundRuns**: one or more run numbers to describe the background.
Background runs are reduced together.

**BackgroundScale**: background intensities are rescaled by this number
when subtracting background from sample.

**OutputBackground**: if populated, a one-histogram
:ref:`MatrixWorkspace2D <MatrixWorkspace2D>` is produced containing the
background intensities versus momentum transfer `Q`. In addition, a
workspace of same name plus suffix `_angle` is produced. The latter workspace
contains intensities versus scattering angle. Rescaling of the
background intensities is not applied when producing these workspaces.

**Vanadium runs**: a set of runs to be reduced jointly provide an incoherent
and isotropic scattering to determine instrument efficiency per detector. If
no vanadium is provided, all detectors are assumed to have the same efficiency

Usage
-----

**Determine powder diffraction pattern:**

.. code-block:: python

    from mantid.simpleapi import BASISPowderDiffraction
    BASISPowderDiffraction(RunNumbers='77754',
                           OutputWorkspace='sample',
                           BackgroundRuns='79745',
                           BackgroundScale=0.80,
                           OutputBackground='background'
                           VanadiumRuns='78378')

.. figure:: /images/BASISPowderDiffraction.png

The color image shows the nine diffraction detectors with intensities in a
log-scale. The second detector from the left features a sharp dip in intensity.

.. categories::

.. sourcelink::


