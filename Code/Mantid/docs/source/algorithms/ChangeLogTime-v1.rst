.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Adds a constant to the times for the requested log. 
See also :ref:`ShiftLogTime <algm-ShiftLogTime>` and :ref:`CorrectLogTimes <algm-CorrectLogTimes>`

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
    print "OriginalTimes: ", original
    print "ModifiedTimes: ", modified


.. testcleanup:: ChangeLogTime

   DeleteWorkspace('w')


Output:

.. testoutput:: ChangeLogTime
   
    OriginalTimes:  [2010-Mar-25 16:09:27.780000000,2010-Mar-25 16:10:01.560998229,2010-Mar-25 16:10:31.514001159,2010-Mar-25 16:11:25.498002319]
    ModifiedTimes:  [2010-Mar-25 16:09:37.780000000,2010-Mar-25 16:10:11.560998229,2010-Mar-25 16:10:41.514001159,2010-Mar-25 16:11:35.498002319]

.. categories::
