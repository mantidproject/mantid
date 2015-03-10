.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm *LoadSampleLogsToCSVFile* exports a specified set of sample logs
, which are stored in a MatrixWorkspace, to a CSV file.
The header for the sample log csv file can also 
be created by this algorithm in a seperate *header* file. 

CSV File format
===============

Sample logs are written to a csv file.   
A tab separates any two adjacent values. 

Each entry of each exported sample log will be an individual entry in the
output CSV file,
except in the situation that two entries with time stamps within time tolerance.

The output CSV file has 2+n columns, where n is the number of sample logs 
to be exported. 

Here is the definition for the columns. 

-  Column 1: Absolute time (with respect to the Unix epoch) in seconds
-  Column 2: Relative to first log entry's time
-  Column 3 to (2 + n): log values in the order determined by input
   *SampleLogNames*

Header file
===========

A sample log header file can be generated optionally.  
It contains theree lines described as below. 

-  Line 1: Test date: [Test date in string]
-  Line 2: Test description: [Description of this log file]
-  Line 3: Header content given by user via input property *Header*.
   Usually it is the column names in the .csv file

Time Zone
=========

The time stamps of sample logs are recorded as UTC time in SNS.
Some users wants to see the exported sample log as the neutron facility's local time.
So the input property 'TimeZone' is for this purpose.

Property *TimeZone* does not support all the time zones
but only those with facilities that use Mantid. 

Here is the list of all time zones that are allowed by this algorithm.
- UTC
- GMT+0
- America/New_York
- Asia/Shanghai
- Australia/Sydney
- Europe/London
- Europe/Paris
- Europe/Copenhagen



Usage
-----

**Example - Export a time series sample log to a csv file:**

.. testcode:: ExExportSampleToTSV

   import os

   nxsfilename = "HYS_11092_event.nxs"
   wsname = "HYS_11092_event"

   defaultdir = config["default.savedirectory"]
   if defaultdir == "":
     defaultdir = config["defaultsave.directory"]
   savefile = os.path.join(defaultdir, "testphase4.txt")

   Load(Filename = nxsfilename,
       OutputWorkspace = wsname,
       MetaDataOnly = True,
       LoadLogs = True)

   ExportSampleLogsToCSVFile(
       InputWorkspace = wsname,
       OutputFilename = savefile,
       SampleLogNames = "Phase1, Phase2, Phase3, Phase4",
       WriteHeaderFile = True,
       Header = "Test sample log: Phase1-Phase4",
       TimeZone = "America/New_York",
       TimeTolerance = 0.01)

   headerfilename = os.path.join(defaultdir, "testphase4_header.txt")

   print "File is created = ", os.path.exists(savefile)
   print "Header file is created = ", os.path.exists(headerfilename)

   # Get the lines of both files
   sfile = open(savefile, 'r')
   slines = sfile.readlines()
   sfile.close()
   hfile = open(headerfilename, 'r')
   hlines = hfile.readlines()
   hfile.close()

   print "Number of lines in File =", len(slines)
   print "Number of lines in Header file =", len(hlines)
   
.. testcleanup:: ExExportSampleToTSV

  os.remove(savefile)
  os.remove(headerfilename)


Output:

.. testoutput:: ExExportSampleToTSV

   File is created =  True
   Header file is created =  True
   Number of lines in File = 36
   Number of lines in Header file = 3

.. categories::
