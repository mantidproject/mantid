.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Adds a constant to the times for the requested log.


.. seealso:: :ref:`ShiftLogTime <algm-ShiftLogTime>` and :ref:`CorrectLogTimes <algm-CorrectLogTimes>`

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: ChangeLogTime

    #Load a workspace
    w=Load('CNCS_7860')
    #get the log times for a particular variable
    original=w.getRun()['Speed5'].times
    #Change the log times
    w=ChangeLogTime(w,LogName='Speed5',TimeOffset='10')
    #get the log times for a particular variable, after change
    modified=w.getRun()['Speed5'].times
    #print times
    print("OriginalTimes:  {}".format(np.datetime_as_string(original, timezone='UTC')))
    print("ModifiedTimes:  {}".format(np.datetime_as_string(modified, timezone='UTC')))


.. testcleanup:: ChangeLogTime

   DeleteWorkspace('w')


Output:

.. testoutput:: ChangeLogTime

    OriginalTimes:  ['2010-03-25T16:09:27.780000000Z' '2010-03-25T16:10:01.560998229Z'
     '2010-03-25T16:10:31.514001159Z' '2010-03-25T16:11:25.498002319Z']
    ModifiedTimes:  ['2010-03-25T16:09:37.780000000Z' '2010-03-25T16:10:11.560998229Z'
     '2010-03-25T16:10:41.514001159Z' '2010-03-25T16:11:35.498002319Z']

.. categories::

.. sourcelink::
