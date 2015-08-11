:orphan:

.. testcode:: mwTest_Python_Standard_Library[10]

   import datetime as dt
   format = '%Y-%m-%dT%H:%M:%S'
   t1 = dt.datetime.strptime('2008-10-12T14:45:52', format)
   print 'Day', t1.day
   print 'Month',t1.month
   print 'Minute',t1.minute
   print 'Second',t1.second
   
   # Define todays date and time
   t2 = dt.datetime.now()
   diff = t2 - t1
   print 'How many days difference?',diff.days

.. testoutput:: mwTest_Python_Standard_Library[10]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Day 12
   Month 10
   Minute 45
   Second 52
   How many days difference? ...


.. testcode:: mwTest_Python_Standard_Library[38]

   import os.path
   
   directory = 'C:/Users/Files'
   file1 = 'run1.txt'
   fullpath = os.path.join(directory, file1)  # Join the paths together in
                                              # the correct manner
   
   # print stuff about the path
   print os.path.basename(fullpath)  # prints 'run1.txt'
   print os.path.dirname(fullpath)  # prints 'C:\Users\Files'
   
   # A userful function is expanduser which can expand the '~' token to a
   # user's directory (Documents and Settings\username on WinXP  and 
   # /home/username on Linux/OSX)
   print os.path.expanduser('~/test') # prints /home/[MYUSERNAME]/test on
                                      # this machine where [MYUSERNAME] is
                                      # replaced with the login

.. testoutput:: mwTest_Python_Standard_Library[38]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   run1.txt
   ...


