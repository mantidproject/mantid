.. _04_run_logs:

========================
Run Info and Sample Logs
========================

Each workspace has a method called `getRun()`, which can be used to access information regarding the experimental run. A full list of the methods available on the returned object is at :ref:`Run`.

The logs recorded during a run are also stored on the Run object and can be accessed using the `getLogData()` method. It has two variants:

.. code-block:: python

	ws = Load('CNCS_7860_event')
	run = ws.getRun()

	#  You can access all the available log properties using the keys method
	print('Sample Logs:', run.keys())

	# With no argument it returns all logs
	all_logs = run.getLogData()

	# Returns the named log, raising an exception if the name is not found
	temperature = run.getLogData('SampleTemp')

	# Use name & value to access the name values
	vals = temperature.value; print('Temp Values:',vals)

Most logs are a time-series property with the attributes described here. The values at each time can be accessed individually or as a collection.


.. figure:: /images/SineWaveLogFiltering.png
   :alt: SineWaveLogFiltering
   :align: right


Filtering
#########

Note that you can use the algorithms :ref:`algm-FilterByLogValue` or :ref:`algm-FilterByTime`.
The LogFilter class can be used to filter logs using other logs. For instance, a log value describes a sine wave (Right-click > **Show Sample logs** on the Loaded workspace below).

Two additional logs mask out the positive and negative portions of this log. We can filter according to either.

.. testcode:: LogFilter

	from mantid.simpleapi import *
	from mantid.kernel import LogFilter

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

Output:

.. testoutput:: LogFilter

	Unfiltered log contains 100 values
	Filtered log contains 50 positive values
	Filtered log contains 50 negative values

Sample Log Algorithms
#####################

As well as the filtering algorithms, there are plenty of other Sample Log operations that are encompassed in algorithms:

- :ref:`algm-CopyLogs`
- :ref:`algm-MergeRuns`
- :ref:`algm-RemoveLogs`
- :ref:`algm-AddTimeSeriesLog`
- :ref:`algm-AddSampleLog`
- :ref:`algm-ExportExperimentLog`
- :ref:`algm-CheckForSampleLogs`
