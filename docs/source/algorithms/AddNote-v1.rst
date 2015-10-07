
.. algorithm::

.. summary::

.. alias::

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

	# Create a host workspace
	ws = CreateSampleWorkspace()
   
	AddNote(ws, Name="my_log", Time="2014-01-01T00:00:00", Value="Initial")
	AddNote(ws, Name="my_log", Time="2014-01-01T00:30:30", Value="Second")
	AddNote(ws, Name="my_log", Time="2014-01-01T00:50:00", Value="Final")
   
	log = ws.getRun().getLogData("my_log")
	print "my_log has %i entries" % log.size()
	for i in range(log.size()):
		print "\t%s\t%s" %(log.times[i], log.value[i])
	
	AddNote(ws, Name="my_log", Time="2014-01-01T00:00:00", Value="New Initial", DeleteExisting=True)
	AddNote(ws, Name="my_log", Time="2014-01-01T00:30:00", Value="New Final")
	
	log = ws.getRun().getLogData("my_log")
	print "my_log now has %i entries" %log.size()
	for i in range(log.size()):
		print "\t%s\t%s" % (log.times[i], log.value[i])
		
Output:

.. testoutput:: AddNoteExample
    :options: +NORMALIZE_WHITESPACE
	
	my_log has 3 entries
            2014-01-01T00:00:00     Initial
            2014-01-01T00:30:30     Second
            2014-01-01T00:50:00     Final
    my_log now has 2 entries
            2014-01-01T00:00:00     New Initial
            2014-01-01T00:30:00     New Final  

.. categories::

