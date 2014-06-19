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
  
  nxsfilename = "/Users/wzz/Mantid/Test/AutoTestData/UsageData/HYS_11092_event.nxs"
  wsname = "HYS_11092_event"
  
  savefile = os.path.join(config["default.savedirectory"], "testphase4.txt")
  print "default save directory: ", config["default.savedirectory"]
  print "File %s created: %s" %(savefile, str(os.path.exists(savefile)))
  
  
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
      
  

.. testcleanup:: ExExportSampleToTSV

  import os

  headerfilename = os.path.join(config["default.savedirectory"], "testphase4_header.txt")
  os.remove(savefile)
  os.remove(headerfilename)


Output:

.. testoutput:: ExExportSampleToTSV




.. categories::
