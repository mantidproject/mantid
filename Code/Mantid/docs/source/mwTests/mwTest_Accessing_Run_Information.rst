:orphan:

.. testcode:: mwTest_Accessing_Run_Information[4]

   ws = Load('CNCS_7860_event')
   run = ws.getRun()
   print run.keys() # You can access all the available log properties using the keys method
   all_logs = run.getLogData() # With no argument it returns all logs
   temperature = run.getLogData('SampleTemp') # Returns the named log, raising an exception if the name is not found
   # Use name & value to access the name values
   vals = temperature.value

.. testoutput:: mwTest_Accessing_Run_Information[4]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ['ChopperStatus1', 'ChopperStatus2', ... 'duration', 'Filename']


.. testsetup:: mwTest_Accessing_Run_Information[27]

   from mantid.kernel import LogFilter

.. testcode:: mwTest_Accessing_Run_Information[27]

   ws = Load("LogWS.nxs")
   run = ws.getRun()
   
   wave_log = run.getLogData('sinewave')
   negative_log = run.getLogData('negative')
   positive_log = run.getLogData('positive')
   
   # Show the size of the original log.
   print "Unfiltered log contains %i values" % (wave_log.size())
   
   # Filter the original log removing negative data
   filter = LogFilter(wave_log)
   filter.addFilter(negative_log)
   filtered_log = filter.data()
   print "Filtered log contains %i positive values" % (filtered_log.size())
   
   # Now filter the original data removing positive values
   filter = LogFilter(wave_log)
   filter.addFilter(positive_log)
   filtered_log = filter.data()
   print "Filtered log contains %i negative values" % (filtered_log.size())

.. testoutput:: mwTest_Accessing_Run_Information[27]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Unfiltered log contains 100 values
   Filtered log contains 50 positive values
   Filtered log contains 50 negative values


