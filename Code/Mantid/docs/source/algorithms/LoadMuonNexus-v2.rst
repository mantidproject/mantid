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
workspace name. For single period data, the optional parameters can be
used to control which spectra are loaded into the workspace. If
spectrum\_min and spectrum\_max are given, then only that range to data
will be loaded. If a spectrum\_list is given than those values will be
loaded.

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

Previous Versions
-----------------

Version 1
#########

Version 1 supports the loading version 1.0 of the muon nexus format.
This is still in active use, if the current version of LoadMuonNexus
detects that it has been asked to load a previous version muon nexus
file it will call the previous version of the algorithm to perform the
task.

.. categories::
