.. _03_run_logs:

==================================
Accessing Run Info and Sample Logs
==================================

Each workspace has a method called `getRun()`, which can be used to access information regarding the experimental run. A full list of the methods available on the returned object is at :ref:<Run>.

The logs recorded during a run are also stored on the Run object and can be accessed using the getLogData method. It has two variants:

.. code-block:: python

	ws = Load('CNCS_7860_event')
	run = ws.getRun()
	print run.keys() # You can access all the available log properties using the keys method
	all_logs = run.getLogData() # With no argument it returns all logs
	temperature = run.getLogData('SampleTemp') # Returns the named log, raising an exception if the name is not found
	# Use name & value to access the name values
	vals = temperature.value

Most logs are a time-series property with the attributes described here. The values at each time can be accessed individually or as a collection.

Filtering
=========

The LogFilter class can be used to filter logs using other logs. For instance, a log value describes a sine wave

#SineWaveLogFiltering.png

Two additional logs mask out the positive and negative portions of this log. We can filter according to either.

.. code-block:: python

	ws = Load("LogWS.nxs")
	run = ws.getRun()

	wave_log = run.getLogData('sinewave')
	negative_log = run.getLogData('negative')
	positive_log = run.getLogData('positive')

	# Show the size of the original log.
	print("Unfiltered log contains %i values" % (wave_log.size()))

	# Filter the original log removing negative data
	filter = LogFilter(wave_log)
	filter.addFilter(negative_log)
	filtered_log = filter.data()
	print("Filtered log contains %i positive values" % (filtered_log.size()))

	# Now filter the original data removing positive values
	filter = LogFilter(wave_log)
	filter.addFilter(positive_log)
	filtered_log = filter.data()
	print("Filtered log contains %i negative values" % (filtered_log.size()))