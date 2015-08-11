:orphan:

.. testcode:: mwTest_Control_Structures_Error_Handling[1]

   # ...
   
   # Execution stopped:
   raise RuntimeError

.. testoutput:: mwTest_Control_Structures_Error_Handling[1]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
   
   RuntimeError


.. testcode:: mwTest_Control_Structures_Error_Handling[14]

   arr = [1,2,3,4,5]
   
   for i in range(6):
       try:
           val = arr[i]
           print str(val)
       except IndexError:
           print 'Error: List index out of range, leaving loop'
           break

.. testoutput:: mwTest_Control_Structures_Error_Handling[14]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   1
   2
   3
   4
   5
   Error: List index out of range, leaving loop


.. Skipping Test  mwTest_Control_Structures_Error_Handling[37]


.. testcode:: mwTest_Control_Structures_Error_Handling[49]

   arr = [1,2,3,4,5]
   value = 0
   try:
      value = arr[5]
   except IndexError:
      print '5 is not a valid array index'
   else:
      print '6th element is ' + str(value)

.. testoutput:: mwTest_Control_Structures_Error_Handling[49]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   5 is not a valid array index


.. testcode:: mwTest_Control_Structures_Error_Handling[65]

   value = 0
   arr = [1,2,3,4]
   element = 6
   try:
       value = arr[element]
   except IndexError:
       print str(element) + ' is not a valid array index'
   else:
       print str(element + 1 ) + 'th element is ' + str(value)
   finally:
       print 'Entered finally clause, do cleanup ...'

.. testoutput:: mwTest_Control_Structures_Error_Handling[65]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   6 is not a valid array index
   Entered finally clause, do cleanup ...


.. testcode:: mwTest_Control_Structures_Error_Handling[88]

   value = 0
   arr = [1,2,3,4]
   element = 6
   try:
       value = arr[element]
   except:     # Catch everything
       print "Something went wrong but I don't know what"

.. testoutput:: mwTest_Control_Structures_Error_Handling[88]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Something went wrong but I don't know what


