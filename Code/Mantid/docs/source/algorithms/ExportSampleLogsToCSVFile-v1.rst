.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm 'LoadSampleLogsToCSVFile' exports a specified set of sample logs
, which are stored in a MatrixWorkspace, to a CSV file.

Each entry of each exported sample log will be an individual entry in the
output CSV file,
except in the situation that two entries with time stamps within time tolerance.


Time Zone
---------

The time stamps of sample logs are recorded as UTC time in SNS.
Some users wants to see the exported sample log as the neutron facility's local time.
So the input property 'TimeZone' is for this purpose.

For 


Header file
-----------

-  Line 0: Test date: [Test date in string]
-  Line 1: Test description: [Description of this log file]
-  Line 2: Header content given by user via input property *Header*.
   Usually it is the column names in the .csv file

CSV File format
---------------

-  Column 0: Absolute time in second
-  Column 1: Relative to first log entry's time
-  Column 2 to (2 + n) - 1: log values in the order determined by input
   *SampleLogNames*

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
