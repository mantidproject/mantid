.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm LoadMuonNexus will read a Muon Nexus data file (original
format) and place the data into the named workspace. The file name can
be an absolute or relative path and should have the extension .nxs or
.NXS. If the file contains data for more than one period, a separate
workspace will be generated for each. After the first period the
workspace names will have "\_2", "\_3", and so on, appended to the given
workspace name. For single period data, the optional parameters can be
used to control which spectra are loaded into the workspace. If
spectrum\_min and spectrum\_max are given, then only that range to data
will be loaded. If a spectrum\_list is given than those values will be
loaded.

-  TODO get XML descriptions of Muon instruments. This data is not in
   existing Muon Nexus files.

Version Identification
######################

To be identified as a version 2 muon nexus file, the nexus file must have a
``IDF_version`` or ``idf_version`` item in its main group
with an integer value of ``2`` and also a
``definition`` item with string value ``"muonTD"`` or ``"pulsedTD"``,
else it will be identified as version 1.

.. _`Version 1 only`:

The following properties apply only to Version 1:

* AutoGroup
* MainFieldDirection
* DeadTimeTable
* DetectorGroupingTable

Time series data
################

The log data in the Nexus file (NX\_LOG sections) will be loaded as
TimeSeriesProperty data within the workspace. Time is stored as seconds
from the Unix epoch.

Errors
######

The error for each histogram count is set as the square root of the
number of counts.

Details of data loaded
######################

Here are more details of data loaded from the Nexus file:


+----------------------------------+--------------------------------------------+----------------------------------------+
| Description of Data              | Found in Nexus file                        | Placed in Workspace (Workspace2D)      |
|                                  | (within the main entry)                    | or output                              |
+==================================+============================================+========================================+
| Detector Data                    | First group of class NXData                | Histogram Data of workspace            |
|                                  | (henceforth referred as [DET])             |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Instrument                       | group ``Instrument``                       | Workspace instrument                   |
|                                  |                                            | as loaded by LoadInstrumentFromNexus   |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Spectrum of each workspace index | [DET]``/spectrum_index``                   | Spectra-Detector mapping of workspace  |
|                                  |                                            | (see below)                            |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Title (optional)                 | ``title``                                  | ``title`` in workspace                 |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Note orcomment (optional)        | ``note``                                   | ``comment`` in workspace               |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Time Zero (optional)             | ``instrument/detector_fb/time_zero``       | TimeZero property                      |
|                                  | if found                                   |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| First good time (optional)       | ``instrument/detector_fb/first_good_time`` | FirstGoodData property                 |
|                                  | if found                                   |                                        |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Run                              | various places as shown later on           | Run object of workspace                |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Sample name                      | ``sample/name``                            | ``name`` in Sample Object of workspace |
+----------------------------------+--------------------------------------------+----------------------------------------+
| Time series                      | ``sample/`` groups of class ``NXLog``      |  time series log in run object         |
+----------------------------------+--------------------------------------------+----------------------------------------+

Run Object
''''''''''
LoadMuonNexus does not run LoadNexusLogs to load run logs. Information is loaded as follows:

+---------------------------+----------------------------------+
| Nexus                     | Workspace run object             |
+===========================+==================================+
| ``title``                 | ``run_title``                    |
+---------------------------+----------------------------------+
| (data)                    | ``nspectra``                     |
+---------------------------+----------------------------------+
| ``start_time``            | ``run_start``                    |
+---------------------------+----------------------------------+
| ``end_time``              | ``run_end``                      |
+---------------------------+----------------------------------+
| ``run/good_total_frames`` | ``goodfrm``                      |
+---------------------------+----------------------------------+
| ``run/number_periods``    | ``nperiods``                     |
+---------------------------+----------------------------------+
| (start & end times)       | ``dur_secs``                     |
+---------------------------+----------------------------------+


(data) indicates that the number is got from the histogram data in an appropriate manner.

Spectra-detector mapping
''''''''''''''''''''''''
Unlike muon v1 NeXus files, v2 files have the possibility that one spectrum maps to multiple detectors.
This mapping is handled as follows:

The spectrum numbers are in ``run/detector_fb/spectrum_index``.

The spectrum ``spectrum_index[j]`` maps to ``detector_count[j]`` detectors.
Their detector IDs are contained in ``detector_list``, starting at the index ``detector_index[j]``.

If the above optional entries ``detector_count``, ``detector_list`` and ``detector_index`` are not present
in the file, it is assumed that the mapping is one-to-one (spectrum number = detector ID). This is the case
for most ISIS data at the moment.


Child Algorithms used
#####################

The ChildAlgorithms used by LoadMuonNexus are:

* :ref:`algm-LoadMuonNexus-v1` - this loads the muon nexus file if not identified as
  version 2. It in turn uses the following child algorithm:
* :ref:`algm-LoadMuonLog` - this reads log information from the Nexus file and uses
  it to create TimeSeriesProperty entries in the workspace.
* :ref:`algm-LoadInstrument` - this algorithm looks for an XML description of the
  instrument and if found reads it.
* :ref:`algm-LoadInstrumentFromNexus` - this is called if the normal
  LoadInstrument fails. As the Nexus file has limited instrument data,
  this only populates a few fields.

Previous Versions
-----------------

Version 1
#########

Version 1 supports the loading version 1.0 of the muon nexus format.
This is still in active use, if the Load algorithm detects that it
has been asked to load a previous version muon nexus file it will
call the previous version of the LoadMuonNexus algorithm to perform the
task.

See :ref:`algm-LoadMuonNexus-v1` for more details about version 1.

Some algorithm properties apply to `Version 1 only`_.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS muon MUSR dataset:**

.. testcode:: LoadMuonNexusOnePeriod

   # Load MUSR dataset
   ws = Load(Filename="MUSR00015189.nxs",EntryNumber=1)
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: LoadMuonNexusOnePeriod

   Workspace has  64  spectra

**Example - Load event nexus file with time filtering:**

.. testcode:: ExLoadMuonNexusSomeSpectra

   # Load some spectra
   ws = Load(Filename="MUSR00015189.nxs",SpectrumMin=5,SpectrumMax=10,EntryNumber=1)
   print("Workspace has  {}  spectra".format(ws[0].getNumberHistograms()))

Output:

.. testoutput:: ExLoadMuonNexusSomeSpectra

   Workspace has  6  spectra

**Example - Load dead times into table:**

.. testcode:: ExLoadDeadTimeTable

   # Load some spectra
   ws = Load(Filename="emu00006473.nxs",SpectrumMin=5,SpectrumMax=10,DeadTimeTable="deadTimeTable")
   tab = mtd['deadTimeTable']
   for i in range(0,tab.rowCount()):
       print("{} {:.12f}".format(tab.cell(i,0), tab.cell(i,1)))

Output:

.. testoutput:: ExLoadDeadTimeTable

   5 0.001611122512
   6 0.002150168177
   7 0.010217159986
   8 0.004316862207
   9 0.007436056621
   10 0.004211476538


.. categories::

.. sourcelink::
