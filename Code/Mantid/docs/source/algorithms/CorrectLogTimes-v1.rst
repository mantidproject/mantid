.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Sometimes the clocks controlling different sample environments or other
experimental log values are not synchronized. This algorithm attempts to
make all (some) time series property logs start at the same time as the
first time in the proton charge log. It uses :ref:`ChangeLogTime <algm-ChangeLogTime>`.

See also :ref:`ShiftLogTime <algm-ShiftLogTime>` and :ref:`ChangeLogTime <algm-ChangeLogTime>`.


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: CorrectLogTimes
    
    w=Load('CNCS_7860')
    print "Original start time for 'proton_charge':", str(w.getRun()['proton_charge'].times[0]).strip()
    print "Original start time for 'Speed5':", str(w.getRun()['Speed5'].times[0]).strip()
    #Change the log times
    CorrectLogTimes(w)
    #there should be almost 10 seconds different than before
    print "Corrected start time for 'Speed5':", str(w.getRun()['Speed5'].times[0]).strip()


.. testcleanup:: CorrectLogTimes

   DeleteWorkspace('w')


Output:

.. testoutput:: CorrectLogTimes

    Original start time for 'proton_charge': 2010-03-25T16:08:37 
    Original start time for 'Speed5': 2010-03-25T16:09:27.780000000 
    Corrected start time for 'Speed5': 2010-03-25T16:08:37 

.. categories::
