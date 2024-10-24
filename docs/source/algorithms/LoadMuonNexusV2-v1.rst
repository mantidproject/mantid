.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm LoadMuonNexusV2 will read a Muon Nexus data file
and place the data into the named workspace. The file name can
be an absolute or relative path and should have the extension .nxs or
.NXS. If spectrum\_min and spectrum\_max are given, then only that range to data
will be loaded. If a spectrum\_list is given than those values will be
loaded.

**Note: Currently only single period loading is supported**

File Identification
######################

To be identified as a Muon nexus V2 file, the nexus file must have the top level entry /raw_data_1
so it can be loaded using the LoadISISNexus child algorithm, see :ref:`algm-LoadISISNexus-v2`.
As well as the raw_data_1 entry, the file should also have an
``IDF_version`` group with value ``2`` and a ``definition`` group, with string value ``"muonTD"`` or ``"pulsedTD"``.


Time series data
################

The log data in the Nexus file (NX\_LOG sections) will be loaded as
TimeSeriesProperty data within the workspace. Time is stored as seconds
from the Unix epoch.

Details of data loaded
######################

In the table below details of the information loaded is shown, for more information see :ref:`algm-LoadISISNexus-v2`.

+----------------------------------+--------------------------------------------+----------------------------------------+
| Description of Data              | Found in Nexus file                        | Placed in Workspace (Workspace2D)      |
|                                  | (within the raw_data_1 entry)              | or output                              |
+==================================+============================================+========================================+
| Detector Data                    | ``/detector_1``                            | Histogram Data of workspace            |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Instrument                       | ``/instrument``                            | Workspace instrument                   |
|                                  |                                            | as loaded by LoadInstrumentFromNexus   |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Spectrum of each workspace index | ``/detector_1/spectrum_index``             | Spectra-Detector mapping of workspace  |
|                                  |                                            | (assume 1-1 mapping)                   |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Time Zero                        | ``instrument/detector_1/time_zero``        | TimeZero property                      |
|                                  | if found                                   |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Time Zero Vector                 | ``instrument/detector_1/time_zero`` can be | TimeZeroList property                  |
|                                  | a single value or an array. This contains  |                                        |
|                                  | either the values from the array or the    |                                        |
|                                  | single value expanded into a vector        |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| First good time                  | ``instrument/detector_1/first_good_bin``   | FirstGoodData property                 |
|                                  | and ``instrument/detector_1/resolution``   |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Nexus logs                       | ``/runlog`` entry                          | Sample logs, loaded using LoadNexusLogs|
+----------------------------------+--------------------------------------------+----------------------------------------+
| Mainfield direction              | ``instrument/detector_1/orientation`` entry| MainFieldDirection Property            |
|                                  | if found, else assumed Longitudinal        |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Deadtime table                   | ``instrument/detector_1/dead_time`` entry  | Deadtime table                         |
+----------------------------------+--------------------------------------------+----------------------------------------+

Run Object
''''''''''
LoadMuonNexusV2 uses LoadNexusLogs :ref:`algm-LoadNexusLogs`.to load the run logs. Information is loaded from the /raw_data_1/runlog entry.
Additionally, LoadMuonNexusV2 loads the sample log entries from /raw_data_1/sample entry, which contains the following:

+---------------------------+----------------------------------+
| Nexus                     | Workspace run object             |
+===========================+==================================+
| magnetic_field            | ``sample_magn_field``            |
+---------------------------+----------------------------------+
| temperature               | ``sample_temp``                  |
+---------------------------+----------------------------------+


Spectra-detector mapping
''''''''''''''''''''''''
The spectrum numbers are in ``run/detector_1/spectrum_index`` and a one-one correspondence between
spectrum number and detector ID is assumed.

Usage
-----

**Example - Load ISIS muon MUSR dataset:**

.. testcode:: LoadMuonNexusV2OnePeriod

   # Load MUSR dataset
   ws = LoadMuonNexusV2(Filename="EMU00102347.nxs_v2")
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: LoadMuonNexusV2OnePeriod

   Workspace has  96  spectra

**Example - Load event nexus file with time filtering:**

.. testcode:: ExLoadMuonNexusV2SomeSpectra

   # Load some spectra
   ws = LoadMuonNexusV2(Filename="EMU00102347.nxs_v2",SpectrumMin=5,SpectrumMax=10)
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: ExLoadMuonNexusV2SomeSpectra

   Workspace has  6  spectra

**Example - Load dead times into table:**

.. testcode:: ExLoadDeadTimeTable

   # Load some spectra
   ws = LoadMuonNexusV2(Filename="EMU00102347.nxs_v2",SpectrumMin=5,SpectrumMax=10,DeadTimeTable="deadTimeTable")
   tab = mtd['deadTimeTable']
   for i in range(0,tab.rowCount()):
       print("{} {:.12f}".format(tab.cell(i,0), tab.cell(i,1)))

Output:

.. testoutput:: ExLoadDeadTimeTable

   5 0.007265590131
   6 0.006881169975
   7 -0.003046069993
   8 0.006345409900
   9 0.007483229972
   10 -0.010110599920

**Example -Time zero loading:**

.. testcode:: ExTimeZeroLoading

   # Load some spectra
   ws, main_field_direction, time_zero, first_good_data, last_good_data, time_zero_list, time_zero_table, dead_time_table, detector_grouping_table = \
      LoadMuonNexusV2(Filename="EMU00102347.nxs_v2",SpectrumMin=5,SpectrumMax=10,DeadTimeTable="deadTimeTable",TimeZeroTable="timeZeroTable")

   print('Single time zero value is {:.2g}'.format(time_zero))
   print("TimeZeroList values are:")
   for timeZero in time_zero_list:
      print(round(timeZero, 2))

Output:

.. testoutput:: ExTimeZeroLoading

   Single time zero value is 0.16
   TimeZeroList values are:
   0.16
   0.16
   0.16
   0.16
   0.16
   0.16

**Example - Test loading a multi period file:**

.. testcode:: LoadMuonNexusV2MultiPeriod

   # Load a multi period file
   load_muon_alg = LoadMuonNexusV2(Filename="EMU00103767.nxs_v2")
   # The workspace is the first return value from the Loader.
   wsGroup = load_muon_alg[0]
   print("Workspace Group has  {}  workspaces".format(wsGroup.getNumberOfEntries()))
   for i in range(wsGroup.getNumberOfEntries()):
       print("Workspace has  {}  spectra".format(wsGroup.getItem(i).getNumberHistograms()))

Output:

.. testoutput:: LoadMuonNexusV2MultiPeriod

   Workspace Group has  4  workspaces
   Workspace has  96  spectra
   Workspace has  96  spectra
   Workspace has  96  spectra
   Workspace has  96  spectra

.. categories::

.. sourcelink::
    :h: Framework/DataHandling/inc/MantidDataHandling/LoadMuonNexusV2.h
    :cpp: Framework/DataHandling/src/LoadMuonNexusV2.cpp
