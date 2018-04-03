.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

**Run numbers**:
The syntax for the run numbers designation allows runs to be segregated
into sets. The semicolon symbol ";" is used to separate the runs into sets.
Runs within each set are jointly reduced. However, when
*Single Crystal Diffraction* is selected, all run numbers provided will be reduced
together.

Examples:

- 2144-2147,2149,2156  is a single set. All runs jointly reduced.

- 2144-2147,2149;2156  is set 2144-2147,2149 and set 2156. The sets are reduced separately from each other (except for *Single Crystal Diffraction*).

**Mask file**: The default mask (BASIS_Mask_default_diff.xml) covers all
inelastic detectors as well as the tips of the diffraction tubes. The file
is in directory /SNS/BSS/shared/autoreduce/new_masks_08_12_2015.

**Lambda Range**: a list containing a minimum and maximum values are required.
Incoming neutrons with a wavelength outside this range will be filtered out.
The default range is [5.86, 6.75].

**OutputWorkspace**: a two-dimensional :ref:`MDHistoWorkspace <MDHistoWorkspace>`
containing the intensities projected onto the reciprocal slice, integrated over
the independent axis *Wproj*. The diffraction pattern can be visualized with
the `SliceViewer <http://www.mantidproject.org/SliceViewer>`_.

**Background runs**: one or more run numbers to describe the background, and a
scaling factor between zero and one.

**Vanadium runs**: a set of runs to be reduced jointly provide an incoherent
and isotropic scattering to determine instrument efficiency per detector. If
no vanadium is provided, all detectors are assumed to have the same efficiency

Determine Single Crystal Diffraction
####################################

Creates a diffraction pattern from a set of runs
implementing a rotational scan of the sample around the vertical axis. The
corresponding goniometer's rotation should be logged under log name
**PsiAngleLog**. Option **PsiOffset** allows user to enter a shift for this
angle.

Sample orientation
^^^^^^^^^^^^^^^^^^

**VectorU**: Vector along k_i, when goniometer is at offset.

**VectorV**: In plane vector perpendicular to VectorU, when goniometer is at
offset.

Diffraction preferences
^^^^^^^^^^^^^^^^^^^^^^^

Most of the scattering occurs in the plane defined by VectorU and VectorV.
Please choose Uproj and Vproj defining a plane that is as close as possible
to the scattering plane.

**Uproj**: Defines the first projection vector. Default (1,0,0).

**Vproj**: Defines the second projection vector. Default (0,1,0).

**Wproj**: Defines the third projection vector. Default (0,0,1).

**Nbins**: the diffraction pattern to be viewed is partitioned into an
Nbins x Nbins grid, each grid rectangle assigned a particular scattered
intensity.

If background is provided, two additional workspaces
are generated. Workspace *_bkg* will contain the scattered intensity by the
background, and workspace *_dat* will contain the scattered intensity by
the sample with *no* background subtracted.

*Timing*: 10 to 15 seconds per sample run. A typical scan made up of 50 runs
will take about 10 minutes to complete.

Usage
-----

**Determine single crystal diffraction pattern:**

.. code-block:: python

    from mantid.simpleapi import BASISDiffraction
    BASISDiffraction(SingleCrystalDiffraction=True,
                     RunNumbers='74799-74869',
                     VanadiumRuns='75524-75526',
                     BackgroundRuns='75527',
                     PsiAngleLog='SE50Rot',
                     PsiOffset=-27.0,
                     LatticeSizes=[10.71, 10.71, 10.71],
                     LatticeAngles=[90.0, 90.0, 90.0],
                          VectorU=[1, 1, 0],
                     VectorV=[0, 0, 1],
                     Uproj=[1, 1, 0],
                     Vproj=[0, 0, 1],
                     Wproj=[1, -1, 0],
                     Nbins=400,
                     OutputWorkspace='peaks')

.. figure:: /images/BASISDiffraction_syngle_crystal_diffraction.png

.. categories::

.. sourcelink::


