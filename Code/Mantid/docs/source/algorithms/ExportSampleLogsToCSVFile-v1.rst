.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

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

**Example - Export a time series sample log to a tsv file:**

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

  print "File is created = ", os.path.exists(savefile), "; file size = ", os.path.getsize(savefile)
  print "Header file is created = ", os.path.exists(headerfilename), "; file size = ", os.path.getsize(headerfilename)

.. testcleanup:: ExExportSampleToTSV

  os.remove(savefile)
  os.remove(headerfilename)


Output:

.. testoutput:: ExExportSampleToTSV


  Input UTC time = 2012-08-14T18:55:52.390000000
  About to convert time string:  2012-08-14T18:55:52
  File is created =  True ; file size =  2583
  Header file is created =  True ; file size =  107

.. categories::
