.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Applies detector grouping to a workspace. (Muon version).

Expect the DetectorGroupingTable to contain one column only. It should
be of type vector\_int (std::vector). Every row corresponds to a group,
and the values in the only column are IDs (not indices!) of the
detectors which spectra should be contained in the group. Name of the
column is not used.

One detector might be in more than one group. Empty groups are ignored.
std::invalid\_argument exceptions are thrown if table format is not
correct, there are no non-empty groups or one of the detector IDs does
not exist in the workspace.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Grouping MUSR data:**

.. testcode:: ExGroupMUSR

   # Load first period of a MUSR data file
   ws = Load('MUSR00015189.nxs', EntryNumber=1)

   # Create a table with some custom grouping
   grouping = CreateEmptyTableWorkspace()
   grouping.addColumn('vector_int', 'detectors')

   # 3 rows = 3 groups
   grouping.addRow([range(1,6)])
   grouping.addRow([range(6,11)])
   grouping.addRow([[2,3,4]])

   grouped = MuonGroupDetectors('ws', 'grouping')

   print('No. of spectra in grouped workspace: {}'.format(grouped.getNumberHistograms()))
   print('Detectors grouped in spectra 0: {}'.format(list(grouped.getSpectrum(0).getDetectorIDs())))
   print('Detectors grouped in spectra 1: {}'.format(list(grouped.getSpectrum(1).getDetectorIDs())))
   print('Detectors grouped in spectra 2: {}'.format(list(grouped.getSpectrum(2).getDetectorIDs())))

Output:

.. testoutput:: ExGroupMUSR

   No. of spectra in grouped workspace: 3
   Detectors grouped in spectra 0: [1, 2, 3, 4, 5]
   Detectors grouped in spectra 1: [6, 7, 8, 9, 10]
   Detectors grouped in spectra 2: [2, 3, 4]


**Example - Using grouping from data file:**

.. testcode:: ExGroupingFromNexus

   # Load MUSR grouping from the Nexus file
   __unused_ws = Load('MUSR00015189.nxs', DetectorGroupingTable='grouping')

   # Load data from a different MUSR file
   ws = Load('MUSR00015190.nxs')

   # Use grouping from one file to group data from different file
   grouped = MuonGroupDetectors('ws', 'grouping')

   print('No. of periods loaded: {}'.format(grouped.size()))
   print('No. of grouped spectra in first period: {}'.format(grouped.getItem(0).getNumberHistograms()))
   print('No. of grouped spectra in second period: {}'.format(grouped.getItem(1).getNumberHistograms()))

Output:

.. testoutput:: ExGroupingFromNexus

   No. of periods loaded: 2
   No. of grouped spectra in first period: 2
   No. of grouped spectra in second period: 2

.. categories::

.. sourcelink::
