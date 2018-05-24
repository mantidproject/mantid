.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The Algorithm is very similar to :ref:`algm-LoadLog`, except that the
source of the data is a Muon Nexus file.

LoadMuonLog loads all the time series logs in the top level of a muon Nexus file (version 1).
These are NX\_LOG entries under the main NX\_ENTRY (usually called ``run``).
Each entry is added to the given workspace's run object as a time series property.

These logs are various time series properties added to the file by SECI.
They include magnetic fields, temperatures, status codes, count rate and beam logs.
(In version 2 muon Nexus files, these are incorporated into the Nexus definition instead).

In the file, times are stored relative to the Unix epoch. The algorithm corrects these
relative to the start time of the run (``run/start_time``) before adding the properties to the workspace.

The algorithm also sets the workspace's sample name from the Nexus file (``run/sample/name``).

Parent Algorithm
################

LoadMuonLog is also a child algorithm of
:ref:`algm-LoadMuonNexus-v1`, i.e. it gets called whenever
LoadMuonNexus (version 1) is executed.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading log data into a dummy workspace:**

.. testcode:: Ex

   # Create a dummy workspace and load in the log data from a file.
   fake_musr_ws = CreateSampleWorkspace()
   LoadMuonLog(fake_musr_ws, "MUSR00015189.nxs")

   # Extract a property from the log.
   time_series_prop = fake_musr_ws.run().getLogData("BEAMLOG_FREQ")

   print("BEAMLOG_FREQ is a TimeSeriesProperty with {} entries.".format(time_series_prop.size()))
   print("The first entry is {:.6f}.".format(time_series_prop.firstValue()))

Output:

.. testoutput:: Ex

   BEAMLOG_FREQ is a TimeSeriesProperty with 10 entries.
   The first entry is 50.000000.

.. categories::

.. sourcelink::
