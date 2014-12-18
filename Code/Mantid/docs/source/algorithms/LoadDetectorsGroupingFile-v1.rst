.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to generate a GroupingWorkspace from an XML or
Map file containing detectors' grouping information.

XML File Format
---------------

Extension: .xml

Parameters
##########

-  "instrument": optional attribute of node 'detector-grouping'. It must
   be valid instrument name. If "instrument" is not defined, only tag
   "ids" can be used in this XML file.
-  "ID": optional attribute of node 'group'. It must be an integer, and
   the key to denote group. If "ID" is not given, algorithm will use
   default group ID for each group in the same order as in XML file. The
   automatic group ID starts from 1.
-  "detids": a node to define grouping by detectors' ID. Its value must
   be a list of integers separated by ','. A '-' is used between 2
   integers to define a range of detectors.
-  "component": a node to define that all detectors belonged to a
   component in the instrument are to be in a same group. Its value
   should be a valid component name.
-  "ids": a node to define that all detectors of the spectrum whose ID
   's defined by "ids" will be grouped together.

Example 1 (using detector IDs and explicit group ID):

.. code-block:: xml

 <?xml version="1.0" encoding="UTF-8" ?>
 <detector-grouping instrument="VULCAN">
  <group ID="4">
   <detids>28750-29981</detids>
   <component>bank21</component>
  <group ID="5">
   <component>bank26</component>
  </group>
 </detector-grouping>

Example 2 (using detector IDs and default group ID):

.. code-block:: xml

 <?xml version="1.0" encoding="UTF-8" ?>
 <detector-grouping instrument="VULCAN">
  <group>
   <detids>28750-29981</detids>
   <component>bank21</component>
  <group>
   <component>bank26</component>
  </group>
 </detector-grouping>

Example 3 (using spectra number - note that no instrument is required):

.. code-block:: xml

 <?xml version="1.0" encoding="UTF-8" ?>
 <detector-grouping>
  <group ID="1">
   <ids>3,34-44,47</ids>
  <group ID="2">
   <ids>26</ids>
   <ids>27,28</ids>
  </group>
 </detector-grouping>

Map File Format
---------------

Extension: .map

The file must have the following format\* (extra space and comments
starting with # are allowed)::

   "unused number1"
   "unused number2"
   "number_of_input_spectra1"
   "input spec1" "input spec2" "input spec3" "input spec4"
   "input spec5 input spec6"
   **
   "unused number2"
   "number_of_input_spectra2"
   "input spec1" "input spec2" "input spec3" "input spec4"



\* each phrase in " " is replaced by a single integer

\*\* the section of the file that follows is repeated once for each
group

Some programs require that "unused number1" is the number of groups
specified in the file but Mantid ignores that number and all groups
contained in the file are read regardless. "unused number2" is in other
implementations the group's spectrum number but in this algorithm it is
is ignored and can be any integer (not necessarily the same integer)

An example of an input file follows::

 3
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
 3
 60
 125 126 127 - 180 181 182 183 184


Usage
-----
.. testcode:: LoadDetectorsGroupingFile

    # create some grouping file
    import mantid
    filename=mantid.config.getString("defaultsave.directory")+"test.xml"
    f=open(filename,'w')
    f.write('<?xml version="1.0" encoding="UTF-8" ?> \n')
    f.write('<detector-grouping instrument="VULCAN"> \n')
    f.write('  <group ID="1"> \n')
    f.write('   <detids>28750-29981</detids> \n')
    f.write('   <component>bank23</component> \n')
    f.write('  </group> \n')
    f.write('  <group ID="2"> \n')
    f.write('   <component>bank26</component> \n')
    f.write('   <component>bank27</component> \n')
    f.write('  </group> \n')
    f.write(' </detector-grouping>')
    f.close()

    #load the grouping file
    ws=LoadDetectorsGroupingFile("test.xml")

    #check some values
    sid=0
    print "Detector "+ws.getDetector(sid).getName()+", with ID "+\
        str(ws.getDetector(sid).getID())+ ", in spectrum "+str(sid)+\
        " belongs to group "+str(int(ws.dataY(sid)[0]))
    sid=2500
    print "Detector "+ws.getDetector(sid).getName()+", with ID "+\
	    str(ws.getDetector(sid).getID())+ ", in spectrum "+str(sid)+\
	    " belongs to group "+str(int(ws.dataY(sid)[0]))
    sid=5000
    print "Detector "+ws.getDetector(sid).getName()+", with ID "+\
	    str(ws.getDetector(sid).getID())+ ", in spectrum "+str(sid)+\
	    " belongs to group "+str(int(ws.dataY(sid)[0]))

.. testcleanup:: LoadDetectorsGroupingFile

   DeleteWorkspace(ws)
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"test.xml"
   os.remove(filename)

Output:

.. testoutput:: LoadDetectorsGroupingFile

    Detector bank23(0,0), with ID 26250, in spectrum 0 belongs to group 1
    Detector bank21(4,4), with ID 28786, in spectrum 2500 belongs to group 1
    Detector bank27(9,0), with ID 33822, in spectrum 5000 belongs to group 2

.. categories::
