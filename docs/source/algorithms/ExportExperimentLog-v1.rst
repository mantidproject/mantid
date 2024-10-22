.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm ExportExperimentLog obtains run information, sample
information and sample log information from a MatrixWorkspace and write
them to a csv file.

File Mode
---------

There are 3 modes to write the experiment log file.


1. "new": A new file will be created with header line;

- If a file with same name exists, then this file will be overwritten;

2. "appendfast": A line of experiment log information will be appended
to an existing file;

-  It is assumed that the log names given are exactly same as those in
   the file, as well as their order;
-  Input property *SampleLogTitles* will be ignored in this option;

3. "append": A line of experiment log information will be appended to an
existing file;

-  The algorithm will check whether the specified log file names, titles
   and their orders are exactly same as those in the file to append to;
-  If any difference is deteced, the old file will be renamed in the
   same directory. And a new file will be generated.

Missing Sample Logs
-------------------

If there is any sample log specified in the properites but does not
exist in the workspace, a zero float value will be put to the experiment
log information line, as the preference of instrument scientist.

Sample Log Operation
--------------------

If the type of a sample log is TimeSeriesProperty, it must be one of the
following 5 types.

-  "min": minimum TimeSeriesProperty's values;
-  "max": maximum TimeSeriesProperty's values;
-  "average": average of TimeSeriesProperty's values;
-  "sum": summation of TimeSeriesProperty's values;
-  "0": first value of TimeSeriesProperty's value.

If the type of a sample log is string and in fact it is a string for
time, then there will an option as

-  "localtime": convert the time from UTC (default) to local time

Otherwise, there is no operation required. For example, log 'duration'
or 'run\_number' does not have any operation on its value. An empty
string will serve for them in property 'SampleLogOperation'.

File format
-----------

There are two types of output file formats that are supported. They are
csv (comma separated) file and tsv (tab separated) file. The csv file
must have an extension as ".csv". If a user gives the name of a log
file, which is in csv format, does not have an extension as .csv, the
algorithm will correct it automatically.

Time Zone
---------

The time stamps of sample logs are recorded as UTC time.
Some users wants to see the exported sample log as the neutron facility's local time.
So the input property 'TimeZone' is for this purpose.

Usage
-----

**Example - Export several experiment logs to a csv file:**

.. testcode:: ExExportExpLogs

  import os

  nxsfilename = "HYS_11092_event.nxs"
  wsname = "HYS_11092_event"

  defaultdir = config["default.savedirectory"]
  if defaultdir == "":
    defaultdir = config["defaultsave.directory"]
  savefile = os.path.join(defaultdir, "testlog.txt")

  Load(Filename = nxsfilename,
      OutputWorkspace = wsname,
      MetaDataOnly = True,
      LoadLogs = True)

  ExportExperimentLog(
      InputWorkspace = wsname,
      OutputFilename = savefile,
      FileMode = "new",
      SampleLogNames = "run_start, run_title",
      SampleLogTitles = "AA, BB",
      SampleLogOperation = "None, None",
      FileFormat = "tab",
      TimeZone = "America/New_York")

  print("File is created =  {}".format(os.path.exists(savefile)))

  # Get lines of file
  sfile = open(savefile, 'r')
  slines = sfile.readlines()
  sfile.close()

  print("Number of lines in File = {}".format(len(slines)))

.. testcleanup:: ExExportExpLogs

  import os
  os.remove(savefile)


Output:

.. testoutput:: ExExportExpLogs

   File is created =  True
   Number of lines in File = 2

.. categories::

.. sourcelink::
