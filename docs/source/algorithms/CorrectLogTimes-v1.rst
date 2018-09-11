.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sometimes the clocks controlling different sample environments or other
experimental log values are not synchronized. This algorithm attempts to
make all (some) time series property logs start at the same time as the
first time in the proton charge log. It uses :ref:`ChangeLogTime <algm-ChangeLogTime>`.

.. seealso:: :ref:`ShiftLogTime <algm-ShiftLogTime>` and :ref:`ChangeLogTime <algm-ChangeLogTime>`.


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: CorrectLogTimes

    w=Load('CNCS_7860')
    run=w.getRun()
    ts = np.datetime_as_string(run['proton_charge'].times[0].astype(np.dtype('M8[s]')), timezone='UTC')
    print("Original start time for 'proton_charge': {}".format(ts).strip())
    ts = np.datetime_as_string(run['Speed5'].times[0].astype(np.dtype('M8[s]')), timezone='UTC')
    print("Original start time for 'Speed5': {}".format(ts).strip())
    #Change the log times
    CorrectLogTimes(w)
    #there should be almost 10 seconds different than before
    ts = np.datetime_as_string(run['Speed5'].times[0].astype(np.dtype('M8[s]')), timezone='UTC')
    print("Corrected start time for 'Speed5': {}".format(ts).strip())


.. testcleanup:: CorrectLogTimes

   DeleteWorkspace('w')


Output:

.. testoutput:: CorrectLogTimes

    Original start time for 'proton_charge': 2010-03-25T16:08:37Z
    Original start time for 'Speed5': 2010-03-25T16:09:27Z
    Corrected start time for 'Speed5': 2010-03-25T16:08:37Z

.. categories::

.. sourcelink::
