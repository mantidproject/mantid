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

Usage
-----

.. include:: ../usagedata-note.txt

Example 1: specifying spectrum numbers
######################################

.. testcode:: ExSpectra

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of spectrum numbers
  group_index = GroupDetectors(ws,SpectraList=[1,3,5],Version=1)

  # The specified spectra are grouped and saved into the same workspace.
  # The returned value is an index in ws wich contains the created group.

  # Check the result
  print 'The workspace still has', ws.getNumberHistograms(),'spectra'
  print 'The group is in the',group_index,'-th spectrum'

  # Get detector IDs in the group
  grp_ids = ws.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

  # The other 2 spectra that were included in the group contain no data
  grp_ids = ws.getSpectrum(2).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(4).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)

  # The rest of the spectra are unchanged
  grp_ids = ws.getSpectrum(1).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(3).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(5).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  print '...'
  grp_ids = ws.getSpectrum(199).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)


Output
^^^^^^

.. testoutput:: ExSpectra

  The workspace still has 200 spectra
  The group is in the 0 -th spectrum
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)
  Number of detectors is 0
  Number of detectors is 0
  Number of detectors is 1
  Number of detectors is 1
  Number of detectors is 1
  ...
  Number of detectors is 1

Example 2: specifying detctor IDs
#################################

.. testcode:: ExDet

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of detctor IDs
  group_index = GroupDetectors(ws,DetectorList=[100,102,104],Version=1)

  # The specified spectra are grouped and saved into the same workspace.
  # The returned value is an index in ws wich contains the created group.

  # Check the result
  print 'The workspace still has', ws.getNumberHistograms(),'spectra'
  print 'The group is in the',group_index,'-th spectrum'

  # Get detector IDs in the group
  grp_ids = ws.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

  # The other 2 spectra that were included in the group contain no data
  grp_ids = ws.getSpectrum(2).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(4).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)

  # The rest of the spectra are unchanged
  grp_ids = ws.getSpectrum(1).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(3).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(5).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  print '...'
  grp_ids = ws.getSpectrum(199).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)

Output
^^^^^^

.. testoutput:: ExDet

  The workspace still has 200 spectra
  The group is in the 0 -th spectrum
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)
  Number of detectors is 0
  Number of detectors is 0
  Number of detectors is 1
  Number of detectors is 1
  Number of detectors is 1
  ...
  Number of detectors is 1

Example 3: specifying workspace indices
#######################################

.. testcode:: ExWii

  # Create a workspace filled with a constant value = 0.3
  ws=CreateSampleWorkspace()
  # Group detectots using a list of workspace indices
  group_index = GroupDetectors(ws,WorkspaceIndexList=[0,2,4],Version=1)

  # The specified spectra are grouped and saved into the same workspace.
  # The returned value is an index in ws wich contains the created group.

  # Check the result
  print 'The workspace still has', ws.getNumberHistograms(),'spectra'
  print 'The group is in the',group_index,'-th spectrum'

  # Get detector IDs in the group
  grp_ids = ws.getSpectrum(0).getDetectorIDs()
  print 'Number of grouped detectors is',len(grp_ids)
  print 'Detector IDs:',  grp_ids

  # The other 2 spectra that were included in the group contain no data
  grp_ids = ws.getSpectrum(2).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(4).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)

  # The rest of the spectra are unchanged
  grp_ids = ws.getSpectrum(1).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(3).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  grp_ids = ws.getSpectrum(5).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)
  print '...'
  grp_ids = ws.getSpectrum(199).getDetectorIDs()
  print 'Number of detectors is',len(grp_ids)

Output
^^^^^^

.. testoutput:: ExWii

  The workspace still has 200 spectra
  The group is in the 0 -th spectrum
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)
  Number of detectors is 0
  Number of detectors is 0
  Number of detectors is 1
  Number of detectors is 1
  Number of detectors is 1
  ...
  Number of detectors is 1

.. categories::
