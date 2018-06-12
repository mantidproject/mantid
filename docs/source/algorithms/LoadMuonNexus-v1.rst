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
workspace name. The optional parameters can be
used to control which spectra are loaded into the workspace. If
SpectrumMin and SpectrumMax are given, then only that range of data
will be loaded. If a SpectrumList is given, then those values will be
loaded. If a range and a list are supplied, the algorithm will
load all the specified spectra.

-  TODO get XML descriptions of Muon instruments. This data is not in
   existing Muon Nexus files.

Spectra-detector mapping
########################

For all v1 muon Nexus files, there is a one-to-one mapping between spectrum
number and detector ID.

Time series data
################

The log data in the Nexus file (NX\_LOG sections) will be loaded as
TimeSeriesProperty data within the workspace. Time (stored as seconds
from the Unix epoch in the Nexus file) is corrected relative to the start
time found in ``run/start_time``.

This time series data comprises various values logged by SECI and includes
magnetic fields, temperatures, status codes, count rate and beam logs.

See :ref:`algm-LoadMuonLog` for more details.

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
information. These are loaded as TableWorkspaces of the format accepted
by :ref:`algm-ApplyDeadTimeCorr` and :ref:`algm-MuonGroupDetectors` accordingly. These are
returned if and only if names are specified for the properties. For
multi-period data workspace groups might be returned, if information in
the Nexus files contains this information for each period.

If the file contains no grouping entry (or the entry is full of zeros), the
grouping will be loaded from the IDF instead. If this also fails, a dummy
grouping will be created. In either case, a message will be displayed
in the log to explain this.

Details of data loaded
######################

Here are more details of data loaded from the Nexus file.
Conventionally, the first NX\_ENTRY is called ``run`` and the first NX\_DATA under this is
called ``histogram_data_1``, and these names are used below.


+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Description of Data                | Found in Nexus file                                  | Placed in Workspace (Workspace2D)              |
|                                    |                                                      | or output                                      |
+====================================+======================================================+================================================+
| Detector data                      | - Times (X): ``run/histogram_data_1/corrected_time`` | Histogram data of workspace                    |
|                                    |                                                      |                                                |
|                                    | - Counts (Y): ``run/histogram_data_1/counts``        |                                                |
|                                    |   (2D array of counts in each time bin per           |                                                |
|                                    |   detector)                                          |                                                |
|                                    |                                                      |                                                |
|                                    | - Errors: square root of counts                      |                                                |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Instrument                         | Name from ``run/instrument/name``                    | Workspace instrument                           |
|                                    |                                                      |                                                |
|                                    | Uses child algorithm LoadInstrument to load          | (as loaded by LoadInstrument from IDF)         |
|                                    | the instrument of that name                          |                                                |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Title (optional)                   | ``run/title``                                        | Workspace title                                |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Comment (optional)                 | ``run/notes``                                        | Workspace comment                              |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Time zero (optional)               | ``run/histogram_data_1/time_zero``                   | *TimeZero* property                            |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| First good data (optional)         | Calculated from first good bin and bin size.         | *FirstGoodData* property                       |
|                                    |                                                      |                                                |
|                                    | - First good bin: ``run/histogram_data_1/counts``    | (First good data - time zero) also goes in     |
|                                    |   (attribute ``first_good_bin``)                     | run object under the name ``FirstGoodData``    |
|                                    |                                                      |                                                |
|                                    | - Bin size: ``run/histogram_data_1/resolution``      |                                                |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Detector grouping table (optional) | ``run/histogram_data_1/grouping``                    | *DetectorGroupingTable* property               |
|                                    |                                                      |                                                |
|                                    | If not present, or invalid, loads from IDF.          |                                                |
|                                    |                                                      |                                                |
|                                    | If that also fails, creates dummy grouping           |                                                |
|                                    | (all detectors in one group).                        |                                                |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Dead time table (optional)         | ``run/instrument/detector/deadtimes``                | *DeadTimeTable* property                       |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Main field direction (optional)    | ``run/instrument/detector/orientation``              | *MainFieldDirection* property                  |
|                                    |                                                      |                                                |
|                                    | Assumed to be longitudinal if not present            | Also in run object as ``main_field_direction`` |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Sample name                        | ``run/sample/name``                                  | Name of sample object                          |
+------------------------------------+------------------------------------------------------+------------------------------------------------+
| Run                                | See below                                            | Run object (see below)                         |
+------------------------------------+------------------------------------------------------+------------------------------------------------+

Run Object
''''''''''
Log values are loaded into the workspace run object as follows:

+-------------------------------------------+-------------------------------+
| Nexus                                     | Workspace run object          |
+===========================================+===============================+
| ``run/title``                             | ``run_title``                 |
+-------------------------------------------+-------------------------------+
| (data)                                    | ``nspectra``                  |
+-------------------------------------------+-------------------------------+
| ``run/start_time``                        | ``run_start``                 |
+-------------------------------------------+-------------------------------+
| ``run/stop_time``                         | ``run_end``                   |
+-------------------------------------------+-------------------------------+
| ``run/duration``                          | ``dur``, ``dur_secs`` (same), |
|                                           | ``durunits`` = 1 (seconds)    |
+-------------------------------------------+-------------------------------+
| ``run/number``                            | ``run_number``                |
+-------------------------------------------+-------------------------------+
| ``run/sample/temperature``                | ``sample_temp``               |
+-------------------------------------------+-------------------------------+
| ``run/sample/magnetic_field``             | ``sample_magn_field``         |
+-------------------------------------------+-------------------------------+
| ``run/instrument/beam/frames_good``       | ``goodfrm`` (number of good   |
| (for single-period data)                  | frames)                       |
|                                           |                               |
| (If ``frames_good`` not present, use      |                               |
| ``frames`` instead.)                      |                               |
+-------------------------------------------+                               |
| ``run/instrument/beam/frames_period_daq`` |                               |
| (for multi-period data)                   |                               |
+-------------------------------------------+-------------------------------+
| Other NX\_LOG entries under ``run``       | time series (via LoadMuonLog) |
+-------------------------------------------+-------------------------------+


ChildAlgorithms used
####################

The ChildAlgorithms used by LoadMuonNexus are:

-  :ref:`algm-LoadMuonLog` - this reads log information from the Nexus file and uses
   it to create TimeSeriesProperty entries in the workspace.
-  :ref:`algm-LoadInstrument` - this algorithm looks for an XML description of the
   instrument and if found reads it.
-  :ref:`algm-LoadInstrumentFromNexus` - this is called if the normal
   LoadInstrument fails. As the Nexus file has limited instrument data,
   this only populates a few fields.

.. categories::

.. sourcelink::
