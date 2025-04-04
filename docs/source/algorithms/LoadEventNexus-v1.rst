.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The LoadEventNeXus algorithm loads data from an `EventNexus file <https://manual.nexusformat.org/classes/base_classes/NXevent_data.html>`_ into an :ref:`EventWorkspace <EventWorkspace>`.
The histogram bin boundaries depend on the setting of NumberOfBins,
which by default will be the whole range of time of flight able to hold all events (in all pixels) split into NumberOfBins linear bins,
and will have their :ref:`units <Unit Factory>` set to time-of-flight.
Since it is an :ref:`EventWorkspace <EventWorkspace>`, it can be rebinned to finer bins with no loss of data.

Child algorithms used
#####################

**Sample logs**, such as motor positions or e.g. temperature vs time, are
also loaded using :ref:`LoadNexusLogs <algm-LoadNexusLogs>`.

**Monitors** are loaded using :ref:`LoadNexusMonitors
<algm-LoadNexusMonitors>`.

**Instrument geometry**

There are a series of approaches for extracting the instrument geometry.
These follow the escalation path as follows:

- Tries to load embedded instrument_xml from the NXinstrument if present
  using :ref:`LoadIDFFromNexus <algm-LoadIDFFromNexus>`.
- Else tries to load embedded nexus geometry from the NXinstrument if present
- Else tries to load the instrument using the name extracted from NXinstrument

The latter two possibilities are achieved via
:ref:`LoadInstrument <algm-LoadInstrument>`

Optional properties
###################

If desired, you can filter out the events at the time of loading, by
specifying minimum and maximum time-of-flight values. This can speed up
loading and reduce memory requirements if you are only interested in a
narrow range of the times-of-flight of your data.

You can specify to load only certain spectra within the file,
using the SpectraMax, SpectraMin and SpectraList properties.
This will load data only matching those restrictions.
At facilities that do not group detectors in hardware such as the SNS,
then this will also equate to the detector IDs.

You may also filter out events by providing the start and stop times, in
seconds, relative to the first pulse (the start of the run).

If you wish to load only a single bank, you may enter its name and no
events from other banks will be loaded.

The Precount option will count the number of events in each pixel before
allocating the memory for each event list. Without this option, because
of the way vectors grow and are re-allocated, it is possible for up to
2x too much memory to be allocated for a given event list, meaning that
your EventWorkspace may occupy nearly twice as much memory as needed.
The pre-counting step takes some time but that is normally compensated
by the speed-up in avoid re-allocating, so the net result is smaller
memory footprint and approximately the same loading time.

Event Compression
#################

When ``CompressTolerance`` is set, the data loaded will be compressed while creating the individual weighted events.
The compression will accumulate events to create a weighted event with no time and cannot be further filtered afterwards.
This parameter is interpreted to be linear tolerance when positive, and logarithmic tolerance when negative.
When ``CompressBinningMode`` is specified, the ``CompressTolerance`` is modified be linear or logarithmic.
This mode does take longer than running ``LoadEventNexus`` without compression,
but reduces the overall memory used during algorithm execution, and memory used by the resulting workspace.
For files that do not have many events, this does not necessarily have an effect other than slowing down loading.

.. note:: The workspace created by ``LoadEventNexus`` with compression are different from those created by ``LoadEventNexus`` without compression then ``CompressedEvents``. The histogram representation will be near identical if the tolerence is selected appropriately.


Veto Pulses
###########

Veto pulses can be filtered out in a separate step using
:ref:`algm-FilterByLogValue`:

``FilterByLogValue(InputWorkspace="ws", OutputWorkspace="ws", LogName="veto_pulse_time", PulseFilter="1")``

Data Loaded from Nexus File
###########################

If `LoadAllLogs` is checked, all the logs in the Nexus files will be loaded directly in the sample logs as they are.
The `LoadLogs` flag will be ignored.
If only `LoadLogs` is checked, only a subset of the logs will be processed and loaded, in the manner described afterward.

The nexus file must have ``/raw_data_1`` or ``/entry`` as its main group and
that group be of type ``NXentry``. It also needs a group of type ``NXevent_data``.

The data is read from each group of type ``NXevent_data``.

If the file has an ``isis_vms_compat`` then it is taken to be an ISIS file and
the data will be modified according to the information obtained from this group.


Here are some tables that show it in more detail:

+------------------------------+-------------------------------------------+-------------------------------------+
| Description of Data          | Found in Nexus file                       | Placed in Workspace (Workspace2D)   |
|                              | (within 'raw_data_1')                     |                                     |
+==============================+===========================================+=====================================+
| Monitor Data                 | groups of Class NXMonitor                 | Monitor Data                        |
|                              | (one monitor per group)                   |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+
| Detector Data                | groups of Class NXevent_data              | Event data                          |
|                              | (one bank per group)                      |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+
| Instrument                   | group ``Instrument``                      | Workspace instrument                |
|                              |                                           | if not overridden                   |
+------------------------------+-------------------------------------------+-------------------------------------+
|                              | If ``isis_vms_compat`` exists,            |                                     |
| Spectrum of each detector ID | ``NSP1``, ``UDET`` and ``SPEC``           | Spectra-Detector mapping            |
|                              | within it,                                |                                     |
|                              | else one spectrum per detector assumed    |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+
| Run                          | mainly as loaded from                     | Run Object                          |
|                              | :ref:`algm-LoadNexusLogs`                 |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+
| Sample                       | If ``isis_vms_compat`` exists,            | Sample Object                       |
|                              | ``SPB`` and ``RSPB`` within               |                                     |
|                              | ``isis_vms_compat``                       |                                     |
|                              | else sample not loaded                    |                                     |
+------------------------------+-------------------------------------------+-------------------------------------+

Invalid Period Logs Error
#########################

There is an issue specific to ISIS, particularly on long runs, where noise on data collection instruments can change the period
of the data. This can cause the period associated with certain data points to exceed the total number of periods we expect.

LoadEventNeXus compares the total number of periods we expect, which can be found in the **periods/number** attribute of the run,
and the greatest period number found in **framelog/period_log/value** attribute of the data. If they do not match, an error message
is raised which explains this problem, and loading of the data will be unsuccessful.

In this case, the data is fundamentally corrupted, so Mantid does not know how to correct this automatically. If you only expect 1 period, there is a script
in the script repository, *user/TomTitcombe/correct_period_logs.py*, which will change all the periods in **framelog/period_log/value** to 1.

If you expect more than 1 periods, there is currently no solution script available through Mantid. You should contact the Mantid team to discuss
the problem and possible solutions.

Sample Object
'''''''''''''

If ``isis_vms_compat`` exists,
then the following sample properties are read from it:

+-------------+-------------------------+
| Nexus       | Workspace sample object |
+=============+=========================+
| ``SPB[2]``  | Geometry flag           |
+-------------+-------------------------+
| ``RSPB[3]`` | Thickness               |
+-------------+-------------------------+
| ``RSPB[4]`` | Height                  |
+-------------+-------------------------+
| ``RSPB[5]`` | Width                   |
+-------------+-------------------------+

This is the same as read by :ref:`algm-LoadISISNexus`.



Usage
-----

.. include:: ../usagedata-note.txt

**Example - Load SNS/ISIS event Nexus file:**

.. testcode:: ExLoadEventNexus

   # Load SNS HYS event dataset
   ws = LoadEventNexus('HYS_11092_event.nxs')

   print("The number of histograms (spectra) is: {}".format(ws.getNumberHistograms()))

Output:

.. testoutput:: ExLoadEventNexus

   The number of histograms (spectra) is: 20480

**Example - Load event nexus file with time filtering:**

.. testcode:: ExLoadEventNexusWithFiltering

   # Load SNS CNCS event dataset between 1 and 2 minutes
   ws = LoadEventNexus('CNCS_7860_event.nxs', FilterByTimeStart=60, FilterByTimeStop=120)

   print("The number of events: {}".format(ws.getNumberEvents()))

Output:

.. testoutput:: ExLoadEventNexusWithFiltering

   The number of events: 29753


.. categories::

.. sourcelink::
