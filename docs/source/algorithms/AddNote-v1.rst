
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates/updates a time-series log entry on a chosen workspace. The given
time stamp and value are appended to the named log entry. If the named
entry does no exist, then a new log is created. A time stamp must be
given in ISO8601 format, e.g. 2010-09-14T04:20:12.

Usage
-----

**Example - AddNote**

.. testcode:: AddNoteExample

        import numpy as np
	# Create a host workspace
	ws = CreateSampleWorkspace()

	AddNote(ws, Name="my_log", Time="2014-01-01T00:00:00", Value="Initial")
	AddNote(ws, Name="my_log", Time="2014-01-01T00:30:30", Value="Second")
	AddNote(ws, Name="my_log", Time="2014-01-01T00:50:00", Value="Final")

	log = ws.getRun().getLogData("my_log")
	print("my_log has {} entries".format(log.size()))
	for time, value in zip(log.times, log.value):
		ts = np.datetime_as_string(time.astype(np.dtype('M8[s]')), timezone='UTC')
		print("\t{}\t{}".format(ts, value))

	AddNote(ws, Name="my_log", Time="2014-01-01T00:00:00", Value="New Initial", DeleteExisting=True)
	AddNote(ws, Name="my_log", Time="2014-01-01T00:30:00", Value="New Final")

	log = ws.getRun().getLogData("my_log")
	print("my_log now has {} entries".format(log.size()))
	for time, value in zip(log.times, log.value):
		ts = np.datetime_as_string(time.astype(np.dtype('M8[s]')), timezone='UTC')
		print("\t{}\t{}".format(ts, value))

Output:

.. testoutput:: AddNoteExample
    :options: +NORMALIZE_WHITESPACE

	my_log has 3 entries
            2014-01-01T00:00:00Z     Initial
            2014-01-01T00:30:30Z     Second
            2014-01-01T00:50:00Z     Final
    my_log now has 2 entries
            2014-01-01T00:00:00Z     New Initial
            2014-01-01T00:30:00Z     New Final

.. categories::
