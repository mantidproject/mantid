.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm is similar to :ref:`ChangeLogTime <algm-ChangeLogTime>`, but instead of a defined time shift of the logs, one gets a shift in the indexes of the specified logs. This will make the log shorter by the specified shift. See also :ref:`ChangeLogTime <algm-ChangeLogTime>` and :ref:`CorrectLogTimes <algm-CorrectLogTimes>`

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: ShiftLogTime
    
    #Load a workspace
    w=Load('CNCS_7860')
    #get the log times for a particular variable
    original=w.getRun()['Speed5'].times
    #Change the log times
    w=ShiftLogTime(w,LogName='Speed5',IndexShift='2')
    #get the log times for a particular variable, after change
    modified=w.getRun()['Speed5'].times
    #print times
    print "OriginalTimes: ", original
    print "ModifiedTimes: ", modified


.. testcleanup:: ShiftLogTime

   DeleteWorkspace('w')


Output:

.. testoutput:: ShiftLogTime
   
    OriginalTimes:  [2010-Mar-25 16:09:27.780000000,2010-Mar-25 16:10:01.560998229,2010-Mar-25 16:10:31.514001159,2010-Mar-25 16:11:25.498002319]
    ModifiedTimes:  [2010-Mar-25 16:10:31.514001159,2010-Mar-25 16:11:25.498002319]
    
    
.. categories::
