.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm LoadMuonNexus will read a Muon Nexus data file (original
format) and place the data into the named workspace. The file name can
be an absolute or relative path and should have the extension .nxs or
.NXS. If the file contains data for more than one period, a separate
workspace will be generated for each. After the first period the
workspace names will have "\_2", "\_3", and so on, appended to the given
workspace name. The optional parameters can be
used to control which spectra are loaded into the workspace. If
SpectrumMin and SpectrumMax are given, then only that range of data
will be loaded. If a SpectrumList is given, then those values will be
loaded. If a range and a list are supplied, the algorithm will 
load all the specified spectra.

-  TODO get XML descriptions of Muon instruments. This data is not in
   existing Muon Nexus files.
-  TODO load the spectra detector mapping. This may be very simple for
   Muon instruments.

Time series data
################

The log data in the Nexus file (NX\_LOG sections) will be loaded as
TimeSeriesProperty data within the workspace. Time is stored as seconds
from the Unix epoch.

Errors
######

The error for each histogram count is set as the square root of the
number of counts.

Time bin data
#############

The *corrected\_times* field of the Nexus file is used to provide time
bin data and the bin edge values are calculated from these bin centre
times.

Multiperiod data
################

To determine if a file contains data from more than one period the field
*switching\_states* is read from the Nexus file. If this value is
greater than one it is taken to be the number of periods, :math:`N_p` of
the data. In this case the :math:`N_s` spectra in the *histogram\_data*
field are split with :math:`N_s/N_p` assigned to each period.

Dead times and detector grouping
################################

Muon Nexus v1 files might contain dead time and detector grouping
informationl. These are loaded as TableWorkspaces of the format accepted
by ApplyDeadTimeCorr and MuonGroupDetectors accordingly. These are
returned if and only if names are specified for the properties. For
multi-period data workspace groups might be returned, if information in
the Nexus files contains this information for each period.

ChildAlgorithms used
####################

The ChildAlgorithms used by LoadMuonNexus are:

-  LoadMuonLog - this reads log information from the Nexus file and uses
   it to create TimeSeriesProperty entries in the workspace.
-  LoadInstrument - this algorithm looks for an XML description of the
   instrument and if found reads it.
-  LoadIntstrumentFromNexus - this is called if the normal
   LoadInstrument fails. As the Nexus file has limited instrument data,
   this only populates a few fields.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load ISIS muon MUSR dataset:**

.. testcode:: LoadMuonNexusOnePeriod

   # Load MUSR dataset
   ws = LoadMuonNexus(Filename="MUSR00015189.nxs",EntryNumber=1)
   print "Workspace has ",  ws[0].getNumberHistograms(), " spectra"

Output:

.. testoutput:: LoadMuonNexusOnePeriod

   Workspace has  64  spectra

**Example - Load event nexus file with time filtering:**

.. testcode:: ExLoadMuonNexusSomeSpectra

   # Load some spectra
   ws = LoadMuonNexus(Filename="MUSR00015189.nxs",SpectrumMin=5,SpectrumMax=10,EntryNumber=1)
   print "Workspace has ",  ws[0].getNumberHistograms(), " spectra"

Output:

.. testoutput:: ExLoadMuonNexusSomeSpectra

   Workspace has  6  spectra

**Example - Load dead times into table:**

.. testcode:: ExLoadDeadTimeTable

   # Load some spectra
   ws = LoadMuonNexus(Filename="emu00006473.nxs",SpectrumMin=5,SpectrumMax=10,DeadTimeTable="deadTimeTable")
   tab = mtd['deadTimeTable']
   for i in range(0,tab.rowCount()):
       print tab.cell(i,0), tab.cell(i,1)

Output:

.. testoutput:: ExLoadDeadTimeTable

   5 0.00161112251226
   6 0.00215016817674
   7 0.0102171599865
   8 0.00431686220691
   9 0.00743605662137
   10 0.00421147653833

**Example - Load detector grouping into table:**

.. testcode:: ExLoadDetectorGrouping

   # Load some spectra
   ws = LoadMuonNexus(Filename="emu00006473.nxs",SpectrumList="1,16,17,32",DetectorGroupingTable="detectorTable")
   tab = mtd['detectorTable']
   for i in range(0,tab.rowCount()):
       print tab.cell(i,0)

Output:

.. testoutput:: ExLoadDetectorGrouping

   [ 1 16]
   [17 32]

.. categories::
