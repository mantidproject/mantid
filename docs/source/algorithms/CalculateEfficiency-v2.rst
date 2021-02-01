.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
Compute the detector efficiency of SANS data. This algorithm is used by
SANS reduction.

The flood is a short for flood field. The flood field is collected from a
sample that scatters uniformly in angle to ensure that every pixel on the
detector has the potential to see the same number of neutrons.
Different materials are currently used at the beamlines to measure the flood
field.  At present, the standard samples used for measuring the flood field
are H2O in a 1 mm path length cell for GP-SANS and Bio-SANS, and a 1 mm sheet
of PMMA for EQ-SANS.

In case the input is a :ref:`WorkspaceGroup <WorkspaceGroup>`, it is processed
entry-by-entry if the `MergeGroup` property is set to False. Otherwise, the input
group is going to be merged and counts will be averaged. In addition, counts
in spectra that are masked in one of the entries of the group but not the other
will be replaced with values from the entry that has this spectrum not masked.
This allows, for example, for removal of the holes in the efficiency map caused
by masking the beam stop, provided at least two measurements are done at different,
e.g. horizontal, offsets.

The relative detector sensitivity is computed the following way

:math:`S(x,y)=\frac{I_{flood}(x,y)}{1/N_{pixels}\sum_{i,j}I_{flood}(i,j)}`

where :math:`I_{flood}(x,y)` is the pixel count of the flood data in pixel (x,y).
If a minimum and/or maximum sensitivity is given, the pixels having an
sensitivity outside the given limits are set to `-inf` (In Mantid `EMPTY_DBL`).

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Compute the detector efficiency from a BioSANS data file:**

.. testcode:: ExEff

   # Load your data file
   workspace = LoadSpice2D('BioSANS_empty_cell.xml')

   # Compute the detector sensitivity
   sensitivity = CalculateEfficiency('workspace', MinThreshold=0.5, MaxThreshold=1.5)

.. testsetup:: ExEff_Groups

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'D22'
   config.appendDataSearchSubDir('ILL/D22/')

.. testcode:: ExEff_Groups

   # Load your data file
   workspace_group = Load('D22_mask_central.nxs,D22_mask_offset.nxs')

   # Compute the detector sensitivity
   sensitivity = CalculateEfficiency(InputWorkspace='workspace_group', MergeGroup=True, MinThreshold=0.5, MaxThreshold=1.5)

.. categories::

.. sourcelink::
