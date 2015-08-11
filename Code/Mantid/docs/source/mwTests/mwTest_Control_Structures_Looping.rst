:orphan:

.. Skipping Test  mwTest_Control_Structures_Looping[7]


.. testcode:: mwTest_Control_Structures_Looping[15]

   for i in range(0,10):
       print i
   # Prints all numbers from 0-9

.. testoutput:: mwTest_Control_Structures_Looping[15]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0
   1
   2
   3
   4
   5
   6
   7
   8
   9


.. testcode:: mwTest_Control_Structures_Looping[32]

   for i in range(0,10,2):
       print i
   # Prints even numbers from 0-9

.. testoutput:: mwTest_Control_Structures_Looping[32]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0
   2
   4
   6
   8


.. testcode:: mwTest_Control_Structures_Looping[47]

   nums = [1,2,3,-1,5,6]
   list_ok = True
   for i in nums:
       if i < 0:
           list_ok = False
           break
   
   if list_ok == False:
       print 'The list contains a negative number'

.. testoutput:: mwTest_Control_Structures_Looping[47]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The list contains a negative number


.. testcode:: mwTest_Control_Structures_Looping[64]

   nums =  [1,2,3,-1,5,6]
   pos_sum = 0
   for i in nums:
       if i < 0:
           continue
       pos_sum += i     # compound assignment means pos_sum = pos_sum + i
   
   print 'Sum of positive numbers is ' + str(pos_sum)

.. testoutput:: mwTest_Control_Structures_Looping[64]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Sum of positive numbers is 17


.. testcode:: mwTest_Control_Structures_Looping[79]

   for i in range(0,10):
       print i
   else:
       print 'done'     # Prints numbers 0-9 and the 'done'

.. testoutput:: mwTest_Control_Structures_Looping[79]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0
   1
   2
   3
   4
   5
   6
   7
   8
   9
   done


.. testcode:: mwTest_Control_Structures_Looping[98]

   for i in range(0,10):
       if i == 5:
           break
       print i
   else:
       print 'done'     # Prints numbers 0-4

.. testoutput:: mwTest_Control_Structures_Looping[98]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0
   1
   2
   3
   4


.. testcode:: mwTest_Control_Structures_Looping[117]

   sum = 0
   while sum < 10:
       sum += 1   # ALWAYS remember to update the loop test or it will
                          # run forever!! 
   
   print sum      # Gives value 10

.. testoutput:: mwTest_Control_Structures_Looping[117]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   10


