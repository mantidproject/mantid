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
start at zero and increase in the order the groups are specified. All
detectors from the grouped spectra will be moved to belong to the new
spectrum.

Not all spectra in the input workspace have to be copied to a group. If
KeepUngroupedSpectra is set to true any spectra not listed will be
copied to the output workspace after the groups in order. If
KeepUngroupedSpectra is set to false only the spectra selected to be in
a group will be used.

To create a single group the list of spectra can be identified using a
list of either spectrum numbers, detector IDs or workspace indices. The
list should be set against the appropriate property.

An input file allows the specification of many groups. The file has the
following format::
 [number of groups in file]
 
 [first group's number]
 [number of spectra in first group]
 [spectrum 1] [spectrum 2] [spectrum 3] [...] [spectrum n]
 
 [second group's number]
 [number of spectra in second group]
 [spectrum 1] [spectrum 2] [spectrum 3] [...] [spectrum n]
 
 [repeat as necessary]

Mantid will still work if the number of groups specified at the start is
incorrect, but other software may not. Mantid will warn you if the value
given is incorrect.

Each group's spectrum number is determined by the group's number. This
behaviour can be overriden by enabling the IgnoreGroupNumber property, in
which case the first group will be numbered 1, and the second 2, and so on.

Blank lines and whitespace in the map file are ignored. Comments may be
entered using a #, like in a Python script.

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

Grouping can also be specified using the GroupingPattern property. Its syntax
is as follows:

The pattern consists of a list of numbers that refer to spectra 0 to n-1 and various operators ',',':','+' and '-'.

To remove spectra, list those you want to keep. The ':' symbol indicates a continuous range of spectra, sparing you the need to list every spectrum.
For example if you have 100 spectra (0 to 99) and want to remove the first and last 10 spectra along with spectrum 12,
you would use the pattern '10,13:89'. This says keep spectrum 10 along with spectra 13 to 89 inclusive.

To add spectra, use '+' to add two spectra or '-' to add a range. For example you may with to add spectrum 10 to 12 and ignore the rest, you would use '10+12'.
If you were adding five groups of 20, you would use '0-19,20-39,40-59,60-79,80-99'.

One could combine the two, for example '10+12,13:89' would list the sum of spectra 10 and 12 followed by spectra 13 to 89.

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

**Example 1: specifying a map file**

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
#######

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

**Example 2: specifying spectrum numbers**

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
#######


.. testoutput:: ExSpectra

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

**Example 3: specifying detctor IDs**

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
#######

.. testoutput:: ExDet

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

**Example 4: specifying workspace indices**

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
#######

.. testoutput:: ExWii

  Number of groups is 1
  The grouped spectrum is a sum 3 input spectra:
  0.9 == 3 * 0.3 == 0.9
  Number of grouped detectors is 3
  Detector IDs: set(100,102,104)

**Example 5: keeping ungrouped spectra**

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
#######

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


**Example 6: Group detectors using spectra list**

.. testcode:: ExGroupDetectorsWithSpectra


   # Create test input
   xx=range(0,10)*10;
   # create spectra with signal equal to spectra number
   yy=[]
   for i in xrange(1,11):
       yy=yy+[i]*10  
       
   ws=CreateWorkspace(DataX=xx,DataY=yy,NSpec=10);  
   # Group detectors
   wsg0 = GroupDetectors(ws,SpectraList=[1,2,3],KeepUngroupedSpectra=True,Behaviour='Sum')
   print "Grouped first 3 spectra results in workspace with {0} spectra and the grouped spectra is spectrum 0:".format(wsg0.getNumberHistograms())
   print wsg0.dataY(0);
   print "First unaffected spectrum is now spectrum 1, former spectrum 4:"   
   print wsg0.dataY(1); 
   print "*********************************************************"
   
   # Group detectors differently   
   wsg1 = GroupDetectors(ws,SpectraList=[2,3,4],KeepUngroupedSpectra=True,Behaviour='Sum')
   print "Grouped 3 spectra starting with second results in workspace with {0} spectra and the grouped spectra is spectrum 0:".format(wsg1.getNumberHistograms())
   print wsg1.dataY(0);
   print "First unaffected spectrum is now spectrum 1, former spectrum 0:"   
   print wsg1.dataY(1); 
   print "*********************************************************"   
   
   # Group detectors in a chain:
   wsg2 = GroupDetectors(wsg0,SpectraList=[4,5,6],KeepUngroupedSpectra=True,Behaviour='Sum')   
   print "Grouped 6 spectra 3x3 twice results in workspace with {0} spectra and the grouped spectra is spectrum 0 and 1:".format(wsg2.getNumberHistograms())
   print wsg2.dataY(0);
   print wsg2.dataY(1);   
   print "First unaffected spectrum is now spectrum 3, former spectrum 7:" 
   print wsg2.dataY(2);
   print "*********************************************************"   

Output:

.. testoutput:: ExGroupDetectorsWithSpectra

   Grouped first 3 spectra results in workspace with 8 spectra and the grouped spectra is spectrum 0:
   [ 6.  6.  6.  6.  6.  6.  6.  6.  6.  6.]
   First unaffected spectrum is now spectrum 1, former spectrum 4:
   [ 4.  4.  4.  4.  4.  4.  4.  4.  4.  4.]
   *********************************************************   
   Grouped 3 spectra starting with second results in workspace with 8 spectra and the grouped spectra is spectrum 0:
   [ 9.  9.  9.  9.  9.  9.  9.  9.  9.  9.]
   First unaffected spectrum is now spectrum 1, former spectrum 0:
   [ 1.  1.  1.  1.  1.  1.  1.  1.  1.  1.]
   *********************************************************
   Grouped 6 spectra 3x3 twice results in workspace with 6 spectra and the grouped spectra is spectrum 0 and 1:
   [ 15.  15.  15.  15.  15.  15.  15.  15.  15.  15.]
   [ 6.  6.  6.  6.  6.  6.  6.  6.  6.  6.]
   First unaffected spectrum is now spectrum 3, former spectrum 7:
   [ 7.  7.  7.  7.  7.  7.  7.  7.  7.  7.]
   *********************************************************

**Example 7: Group detectors using map file:**
   
.. testcode:: ExGroupDetectorsWithMap

   import os
   # Create test input
   xx=range(0,10)*10;
   # create spectra with signal equal to spectrum number
   yy=[]
   for i in xrange(1,11):
       yy=yy+[i]*10  
       
   ws=CreateWorkspace(DataX=xx,DataY=yy,NSpec=10);  
   
   # Create map file
   file_name = os.path.join(config["defaultsave.directory"], "TestMapFile.map") 
   f=open(file_name,'w');
   f.write('4\n'); # header, four groups
   f.write('1\n3\n'); # header group 1
   f.write('1 2 3\n'); #  group 1   
   f.write('2\n3\n'); # header group 2   
   f.write('4 5 6\n'); #  group 2
   f.write('3\n2\n'); # header group 3
   f.write('7 8\n'); #  group 3
   f.write('4\n2\n'); # header group 4
   f.write('9 10\n'); #  group 4
   f.close()
     
   # Group detectors
   wsg = GroupDetectors(ws,MapFile=file_name,KeepUngroupedSpectra=True,Behaviour='Sum')
  
   print "Grouped workspace has {0} spectra".format(wsg.getNumberHistograms())
   print "spectrum 1 (sum of spectra 1-3):",wsg.dataY(0)
   print "spectrum 2 (sum of spectra 4-6):",wsg.dataY(1)   
   print "spectrum 3 (sum of spectra 7-8):",wsg.dataY(2)      
   print "spectrum 4 (sum of spectra 9-10):",wsg.dataY(3)
   
.. testcleanup:: ExGroupDetectorsWithMap

   os.remove(file_name)   
 
Output:

.. testoutput:: ExGroupDetectorsWithMap

   Grouped workspace has 4 spectra
   spectrum 1 (sum of spectra 1-3): [ 6.  6.  6.  6.  6.  6.  6.  6.  6.  6.]
   spectrum 2 (sum of spectra 4-6): [ 15.  15.  15.  15.  15.  15.  15.  15.  15.  15.]
   spectrum 3 (sum of spectra 7-8): [ 15.  15.  15.  15.  15.  15.  15.  15.  15.  15.]
   spectrum 4 (sum of spectra 9-10): [ 19.  19.  19.  19.  19.  19.  19.  19.  19.  19.]

**Example 8: Group detectors using grouping pattern:**

.. testcode:: ExGroupDetectorsWithPattern

   # Create Workspace of 5 spectra each with two entries.
   ws = CreateWorkspace(DataX=[1,2], DataY=[11,12,21,22,31,32,41,42,51,52], NSpec=5)

   # Run algorithm adding first two spectra and then keeping last two spectra
   ws2 = GroupDetectors(ws, GroupingPattern="0+1,3,4")

   #print result
   print ws2.readY(0)
   print ws2.readY(1)
   print ws2.readY(2)

Output:

.. testoutput:: ExGroupDetectorsWithPattern

   [ 32.  34.]
   [ 41.  42.]
   [ 51.  52.]

.. categories::
