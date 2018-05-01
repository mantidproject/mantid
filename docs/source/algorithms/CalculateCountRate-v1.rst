.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm works with event workspaces produced by instruments operating in event mode and calculates the rate at which an instrument counts
neutrons as function of the experiment time. 
Then it adds the time series log containing this rate, with the name, defined by 
**CountRateLogName** property to the source workspace. 

Additionally it can also calculate 2D matrix workspace, which contains count rate as function of 
experiment time and neutrons time of flight or energy or other neutron property related to the time of flight in the range **XMin-XMax** 
in the units defined by **RangeUnits** property. 

In normal circumstances the instrument count rate does not change. Unfortunately, Data Acquisition Electronics sometimes randomly generates spurious signals, 
appearing randomly at some moments of the experiment time. Such signals distort real physical image, obtained in experiment and should be removed. 

The picture below gives example of counting rate log, calculated by the algorithm together with image of the visualization workspace, 
which shows the spurious signal as function of experiment time and neutron energy.

.. image:: /images/SpurionReal.png 

The calculated log above can be used as input for :ref:`algm-FilterByLogValue` algorithm to filter events recorded around 200sec of experiment time to
remove spurious signal at 2.9mEv.

The algorithm can also be used to evaluate changes of sample reflectivity as function of some slow changing experiment's 
parameter e.g. temperature, magnetic field or pressure.


**NOTE:** Normally one wants to divide the count rate by a value, proportional to incident neutron flux to avoid count rate dependency from changing incident beam intensity. 
Usually algorithm just divides the neutron counts by *proton_charge* log values, normally recorded per frame basis. 
If this log is not available, other logs can be used to estimate incident beam intensity. 
Visualization workspace is usually calculated with much more coarse time step than the count_rate log, so the normalization used in visualization workspace 
is just the sum of all values of the log used for normalization falling within the visualization time step. 



Usage
-----


**Example - Calculate Count Rate**

.. testcode:: ExCalcCountRate

   if "LogTest" in mtd:
       DeleteWorkspace("LogTest")
   # Create sample workspace with events   
   LogTest=CreateSampleWorkspace(WorkspaceType='Event', Function='Flat background')
   log_name = 'block_count_rate';
   
   # no rate log found:
   rez = CheckForSampleLogs(LogTest,log_name)
   print("Initially, {0}".format(rez))
   
   # calculate number of events in the workspace
   CalculateCountRate(LogTest,CountRateLogName= log_name)
   
   rez = CheckForSampleLogs(LogTest,log_name)   
   if len(rez)==0:
        print("The Algorithm produced log: {0}".format(log_name))        
        log = LogTest.run().getLogData(log_name)
        print("log {0} contains {1} entries".format(log_name, log.size()))
        print("log starts at {0} and records value: {1}".format(log.firstTime(), log.firstValue()))
        print("log ends   at {0} and records value: {1}".format(log.lastTime(), log.lastValue()))
   else:
       print("{0}".format(rez))

   
.. testoutput:: ExCalcCountRate
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    Initially, Property block_count_rate not found
    The Algorithm produced log: block_count_rate
    log block_count_rate contains 200 entries
    log starts at 2010-01-01T00:00:09... and records value: ...
    log ends   at 2010-01-01T00:59:50... and records value: ...
 
.. categories::

.. sourcelink::
