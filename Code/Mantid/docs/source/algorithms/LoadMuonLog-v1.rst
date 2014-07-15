.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Algorithm is very similar to :ref:`algm-LoadLog`, except that the
source of the data is a Muon Nexus file.

Parent Algorithm
################

LoadMuonLog is also a child algorithm of
:ref:`algm-LoadMuonNexus`, i.e. it gets called whenever
LoadMuonNexus is executed.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Loading Log Data into Dummy a Workspace:**

.. testcode:: Ex

   # Create a dummy workspace and load in the log data from a file.
   fake_emu_ws = CreateSampleWorkspace()
   LoadMuonLog(fake_emu_ws, "MUSR00015189.nxs")

   # Extract a property from the log.
   time_series_prop = fake_emu_ws.run().getLogData("BEAMLOG_FREQ")

   print "BEAMLOG_FREQ is a TimeSeriesProperty with %i entries." % time_series_prop.size()
   print "The first entry is %f." % time_series_prop.firstValue()

Output:

.. testoutput:: Ex

   BEAMLOG_FREQ is a TimeSeriesProperty with 10 entries.
   The first entry is 50.000000.

.. categories::
