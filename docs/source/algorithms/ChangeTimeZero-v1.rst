.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm modifies the time signatures of a :ref:`MatrixWorkspace <MatrixWorkspace>`. The new time signature 
alters the logs and in case of an :ref:`EventWorkspace <EventWorkspace>` the neutron times as well. 

The time offset can be specified in one of the two following ways:

*  A time offset in seconds: In this case all time stamps in the workspace are shifted by the specified amount. A positive entry creates a shift into the future and a negative one creates a shift into the past relative to the original time.
*  An ISO8601 time stamp (YYYY-MM-DDTHH:MM:SS, eg 2003-11-30T03:23:54). The logs need to contain a proton_charge time series property for this shift to work. The first time entry of the proton_charge time series is used as a reference time stamp and all times will be shifted according to the differnce between this time stamp and the newly specified value.
 
Only one of the two ways of shifting the time can be specified.
 
Usage
-----

.. include:: ../usagedata-note.txt

**Example - Relative Time Shift for an EventWorkspace:**

.. testcode:: ExRelativeChangeTimeZero
    
   # Load an event workspace 
   original_ws = Load('CNCS_7860_event.nxs')

   # Specify the time shift
   time_shift = 10000.5

   # Change the zero time
   shifted_ws = ChangeTimeZero(InputWorkspace = original_ws , RelativeTimeOffset = time_shift)

   # Check some logs 
   original_proton_charge = original_ws.getRun()['proton_charge']
   shifted_proton_charge = shifted_ws .getRun()['proton_charge']

   # Check some events
   original_pulse_times = original_ws.getSpectrum(7).getPulseTimes()
   shifted_pulse_times = shifted_ws.getSpectrum(7).getPulseTimes()

   print("Original proton_charge time:  {} ,  {} , ...".format(original_proton_charge.nthTime(0), original_proton_charge.nthTime(1)))
   print("Shifted proton_charge time:  {} ,  {} , ...".format(shifted_proton_charge.nthTime(0), shifted_proton_charge.nthTime(1)))

   print("Original pulse times:  {} ,  {} , ...".format(original_pulse_times[0], original_pulse_times[1]))
   print("Shifted pulse times:  {} ,  {} , ...".format(shifted_pulse_times[0], shifted_pulse_times[1]))
   
.. testcleanup:: ExRelativeChangeTimeZero

   DeleteWorkspace('original_ws')


Output:

.. testoutput:: ExRelativeChangeTimeZero

  Original proton_charge time:  2010-03-25T16:08:37  ,  2010-03-25T16:08:37.016667999  , ...
  Shifted proton_charge time:  2010-03-25T18:55:17.500000000  ,  2010-03-25T18:55:17.516667999  , ...
  Original pulse times:  2010-03-25T16:09:57.474029541  ,  2010-03-25T16:10:33.529106140  , ...
  Shifted pulse times:  2010-03-25T18:56:37.974029541  ,  2010-03-25T18:57:14.029106140  , ...
    
**Example - Absolute Time Shift for an EventWorkspace:**

.. testcode:: ExAbsoluteChangeTimeZero

   # Load an event workspace 
   original_ws = Load('CNCS_7860_event.nxs')

   # Specify the time shift
   time_shift = '2002-07-13T14:00:01'

   # Change the zero time
   shifted_ws = ChangeTimeZero(InputWorkspace = original_ws , AbsoluteTimeOffset = time_shift)

   # Check some logs 
   original_proton_charge = original_ws.getRun()['proton_charge']
   shifted_proton_charge = shifted_ws .getRun()['proton_charge']

   # Check some events
   original_pulse_times = original_ws.getSpectrum(7).getPulseTimes()
   shifted_pulse_times = shifted_ws.getSpectrum(7).getPulseTimes()

   print("Original proton_charge time:  {} ,  {} , ...".format(original_proton_charge.nthTime(0), original_proton_charge.nthTime(1)))
   print("Shifted proton_charge time:  {} ,  {} , ...".format(shifted_proton_charge.nthTime(0), shifted_proton_charge.nthTime(1)))

   print("Original pulse times:  {} ,  {} , ...".format(original_pulse_times[0], original_pulse_times[1]))
   print("Shifted pulse times:  {} ,  {} , ...".format(shifted_pulse_times[0], shifted_pulse_times[1]))

.. testcleanup:: ExAbsoluteChangeTimeZero

   DeleteWorkspace('original_ws')


Output:

.. testoutput:: ExAbsoluteChangeTimeZero
 
  Original proton_charge time:  2010-03-25T16:08:37  ,  2010-03-25T16:08:37.016667999  , ...
  Shifted proton_charge time:  2002-07-13T14:00:01  ,  2002-07-13T14:00:01.016667999  , ...
  Original pulse times:  2010-03-25T16:09:57.474029541  ,  2010-03-25T16:10:33.529106140  , ...
  Shifted pulse times:  2002-07-13T14:01:21.474029541  ,  2002-07-13T14:01:57.529106140  , ...
  
.. categories::

.. sourcelink::
