.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm sums, bin-by-bin, multiple spectra into a single spectra.
The errors are summed in quadrature and the algorithm checks that the
bin boundaries in X are the same. The new summed spectra are created at
the start of the output workspace and have spectra index numbers that
start at zero and increase in the order the groups are specified. Each
new group takes the spectra numbers from the first input spectrum
specified for that group. All detectors from the grouped spectra will be
moved to belong to the new spectrum.

Not all spectra in the input workspace have to be copied to a group. If
KeepUngroupedSpectra is set to true any spectra not listed will be
copied to the output workspace after the groups in order. If
KeepUngroupedSpectra is set to false only the spectra selected to be in
a group will be used.

To create a single group the list of spectra can be identified using a
list of either spectrum numbers, detector IDs or workspace indices. The
list should be set against the appropriate property.

An input file allows the specification of many groups. The file must
have the following format\* (extra space and comments starting with #
are allowed)::

 "unused number1"
 "unused number2"
 "number_of_input_spectra1"
 "input spec1" "input spec2" "input spec3" "input spec4"
 "input spec5 input spec6"
 **
 "unused number2"
 "number_of_input_spectra2"
 "input spec1" "input spec2" "input spec3" "input spec4"

\* each phrase in "" is replaced by a single integer

\*\* the section of the file that follows is repeated once for each
group

Some programs require that "unused number1" is the number of groups
specified in the file but Mantid ignores that number and all groups
contained in the file are read regardless. "unused number2" is in other
implementations the group's spectrum number but in this algorithm it is
is ignored and can be any integer (not necessarily the same integer)

An example of an input file follows::

 2
 1
 64
 1 2 3 4 5 6 7 8 9 10
 11 12 13 14 15 16 17 18 19 20
 21 22 23 24 25 26 27 28 29 30
 31 32 33 34 35 36 37 38 39 40
 41 42 43 44 45 46 47 48 49 50
 51 52 53 54 55 56 57 58 59 60
 61 62 63 64
 2
 60
 65 66 67 68 69 70 71 72 73 74
 75 76 77 78 79 80 81 82 83 84
 85 86 87 88 89 90 91 92 93 94
 95 96 97 98 99 100 101 102 103 104
 105 106 107 108 109 110 111 112 113 114
 115 116 117 118 119 120 121 122 123 124

In addition the following XML grouping format is also supported

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <detector-grouping> 
      <group name="fwd1"> <ids val="1-32"/> </group> 
      <group name="bwd1"> <ids val="33,36,38,60-64"/> </group>   

      <group name="fwd2"><detids val="1,2,17,32"/></group> 
      <group name="bwd2"><detids val="33,36,38,60,64"/> </group> 
    </detector-grouping>

where is used to specify spectra IDs and detector IDs.

Previous Versions
-----------------

Version 1
#########

The set of spectra to be grouped can be given as a list of either
spectrum numbers, detector IDs or workspace indices. The new, summed
spectrum will appear in the workspace at the first workspace index of
the pre-grouped spectra (which will be given by the ResultIndex property
after execution). The detectors for all the grouped spectra will be
moved to belong to the first spectrum. *A technical note: the workspace
indices previously occupied by summed spectra will have their data
zeroed and their spectrum number set to a value of -1.* `See page
for version 1 here. <GroupDetectors-v1.html>`_

Usage
-----

Example 1: specifying a map file
################################

.. testcode:: ExMapFile

  import os

  # Create a grouping file from the example above.
  # It makes 2 groups of 64 and 60 detectors respectively.
  groupingFileContent = \
  """
  2
  1
  64
  1 2 3 4 5 6 7 8 9 10
  11 12 13 14 15 16 17 18 19 20
  21 22 23 24 25 26 27 28 29 30
  31 32 33 34 35 36 37 38 39 40
  41 42 43 44 45 46 47 48 49 50
  51 52 53 54 55 56 57 58 59 60
  61 62 63 64
  2
  60
  65 66 67 68 69 70 71 72 73 74
  75 76 77 78 79 80 81 82 83 84
  85 86 87 88 89 90 91 92 93 94
  95 96 97 98 99 100 101 102 103 104
  105 106 107 108 109 110 111 112 113 114
  115 116 117 118 119 120 121 122 123 124
  """
  # Save the data to a file
  groupingFilePath = os.path.expanduser('~/MantidUsageExample_GroupDetectorsGrouping.txt')
  f = open(groupingFilePath, 'w')
  f.write( groupingFileContent )
  f.close()

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectors according to the created file.
  grouped = GroupDetectors( ws, MapFile = groupingFilePath )

  # Check the result
  print 'Number of groups is', grouped.getNumberHistograms()
  print 'First grouped spectrum is a sum 64 input spectra:'
  print  grouped.readY(0)[0],'== 64 * 0.3 ==', 64 * 0.3
  print 'Second grouped spectrum is a sum 60 input spectra:'
  print  grouped.readY(1)[0],'== 60 * 0.3 ==', 60 * 0.3
  # Get detector IDs of the first group
  grp0_ids = grouped.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp0_ids)
  print '5 first detectors in group:', [ grp0_ids[i] for i in range(5) ]
  print '5 last  detectors in group:', [ grp0_ids[i] for i in range(59,64) ]
  # Get detector IDs of the second group
  grp1_ids = grouped.getSpectrum(1).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp1_ids)
  print '5 first detectors in group:', [ grp1_ids[i] for i in range(5) ]
  print '5 last  detectors in group:', [ grp1_ids[i] for i in range(55,60) ]

Output
^^^^^^

.. testoutput:: ExMapFile

  Number of groups is 2
  First grouped spectrum is a sum 64 input spectra:
  19.2 == 64 * 0.3 == 19.2
  Second grouped spectrum is a sum 60 input spectra:
  18.0 == 60 * 0.3 == 18.0
  Number of grouped detectors is 64
  5 first detectors in group: [100, 101, 102, 103, 104]
  5 last  detectors in group: [159, 160, 161, 162, 163]
  Number of grouped detectors is 60
  5 first detectors in group: [164, 165, 166, 167, 168]
  5 last  detectors in group: [219, 220, 221, 222, 223]

.. testcleanup:: ExMapFile

  os.remove( groupingFilePath )

Example 2: specifying spectrum numbers
######################################

.. testcode:: ExSpectra

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of spectrum numbers
  grouped = GroupDetectors(ws,SpectraList=[1,3,5])

  # Check the result
  print 'Number of groups is', grouped.getNumberHistograms()
  print 'The grouped spectrum is a sum 3 input spectra:'
  print  grouped.readY(0)[0],'== 3 * 0.3 ==',3 * 0.3

  # Get detector IDs in the group
  grp_ids = grouped.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

Output
^^^^^^

.. testoutput:: ExSpectra

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

Example 3: specifying detctor IDs
#################################

.. testcode:: ExDet

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of detctor IDs
  grouped = GroupDetectors(ws,DetectorList=[100,102,104])

  # Check the result
  print 'Number of groups is', grouped.getNumberHistograms()
  print 'The grouped spectrum is a sum 3 input spectra:'
  print  grouped.readY(0)[0],'== 3 * 0.3 ==',3 * 0.3

  # Get detector IDs in the group
  grp_ids = grouped.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

Output
^^^^^^

.. testoutput:: ExDet

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

Example 4: specifying workspace indices
#######################################

.. testcode:: ExWii

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of workspace indices
  grouped = GroupDetectors(ws,WorkspaceIndexList=[0,2,4])

  # Check the result
  print 'Number of groups is', grouped.getNumberHistograms()
  print 'The grouped spectrum is a sum 3 input spectra:'
  print  grouped.readY(0)[0],'== 3 * 0.3 ==',3 * 0.3

  # Get detector IDs in the group
  grp_ids = grouped.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

Output
^^^^^^

.. testoutput:: ExWii

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

Example 5: keeping ungrouped spectra
####################################

.. testcode:: ExKeep

  import os

  # Create a grouping file from the example above.
  # It makes 2 groups of 64 and 60 detectors respectively.
  groupingFileContent = \
  """
  2
  1
  64
  1 2 3 4 5 6 7 8 9 10
  11 12 13 14 15 16 17 18 19 20
  21 22 23 24 25 26 27 28 29 30
  31 32 33 34 35 36 37 38 39 40
  41 42 43 44 45 46 47 48 49 50
  51 52 53 54 55 56 57 58 59 60
  61 62 63 64
  2
  60
  65 66 67 68 69 70 71 72 73 74
  75 76 77 78 79 80 81 82 83 84
  85 86 87 88 89 90 91 92 93 94
  95 96 97 98 99 100 101 102 103 104
  105 106 107 108 109 110 111 112 113 114
  115 116 117 118 119 120 121 122 123 124
  """
  # Save the data to a file
  groupingFilePath = os.path.expanduser('~/MantidUsageExample_GroupDetectorsGrouping.txt')
  f = open(groupingFilePath, 'w')
  f.write( groupingFileContent )
  f.close()

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectors according to the created file.
  grouped = GroupDetectors( ws, MapFile=groupingFilePath, KeepUngroupedSpectra=True )

  # Check the result
  print 'Number of spectra in grouped workspace is', grouped.getNumberHistograms()
  print 'It includes 2 groups + ',ws.getNumberHistograms() - (64 + 60),'remaining ungrouped spectra'

  print 'First  spectrum is grouped, it has',len(grouped.getSpectrum(0).getDetectorIDs()),'detectors'
  print 'Second spectrum is grouped, it has',len(grouped.getSpectrum(1).getDetectorIDs()),'detectors'
  print 'Spectrum   2  is ungrouped, it has ',len(grouped.getSpectrum(2).getDetectorIDs()),'detector'
  print 'Spectrum   3  is ungrouped, it has ',len(grouped.getSpectrum(3).getDetectorIDs()),'detector'
  print '...'
  print 'Spectrum  77  is ungrouped, it has ',len(grouped.getSpectrum(77).getDetectorIDs()),'detector'

Output
^^^^^^

.. testoutput:: ExKeep

  Number of spectra in grouped workspace is 78
  It includes 2 groups +  76 remaining ungrouped spectra
  First  spectrum is grouped, it has 64 detectors
  Second spectrum is grouped, it has 60 detectors
  Spectrum   2  is ungrouped, it has  1 detector
  Spectrum   3  is ungrouped, it has  1 detector
  ...
  Spectrum  77  is ungrouped, it has  1 detector

.. testcleanup:: ExKeep

  os.remove( groupingFilePath )

.. categories::
