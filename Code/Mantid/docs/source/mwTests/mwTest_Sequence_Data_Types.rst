:orphan:

.. testcode:: mwTest_Sequence_Data_Types[6]

   lottery_numbers = [1,2,3,4,5,6]


.. testsetup:: mwTest_Sequence_Data_Types[12]

   lottery_numbers = [1,2,3,4,5,6]

.. testcode:: mwTest_Sequence_Data_Types[12]

   bonus = 7
   lottery_numbers.append(bonus)


.. testsetup:: mwTest_Sequence_Data_Types[22]

   lottery_numbers = [1,2,3,4,5,6,7]

.. testcode:: mwTest_Sequence_Data_Types[22]

   # print the first element
   print lottery_numbers[0]  # prints 1
   # print the last element
   print lottery_numbers[6]  # prints 7

.. testoutput:: mwTest_Sequence_Data_Types[22]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   1
   7


.. testsetup:: mwTest_Sequence_Data_Types[38]

   lottery_numbers = [1,2,3,4,5,6,7]

.. testcode:: mwTest_Sequence_Data_Types[38]

   print lottery_numbers  # prints  [1,2,3,4,5,6, 7]
   lottery_numbers.remove(5) # Removes first occurrence of value 5 in list
   del lottery_numbers[0]  # Removes the 0th element of the list
   print lottery_numbers # prints [2,3,4,6,7]

.. testoutput:: mwTest_Sequence_Data_Types[38]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   [1, 2, 3, 4, 5, 6, 7]
   [2, 3, 4, 6, 7]


.. testsetup:: mwTest_Sequence_Data_Types[54]

   lottery_numbers = [2,3,4,6,7]

.. testcode:: mwTest_Sequence_Data_Types[54]

   lottery_numbers[3] = 42
   print lottery_numbers  # gives [2, 3, 4, 42, 7]

.. testoutput:: mwTest_Sequence_Data_Types[54]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   [2, 3, 4, 42, 7]


.. testcode:: mwTest_Sequence_Data_Types[74]

   my_list = ['M','A', 'N', 'T', 'I', 'D']
   print my_list[1:4]  # prints ['A', 'N', 'T']

.. testoutput:: mwTest_Sequence_Data_Types[74]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ['A', 'N', 'T']


.. testcode:: mwTest_Sequence_Data_Types[83]

   my_string = 'MANTID'
   print my_string[1:4]   # prints ANT

.. testoutput:: mwTest_Sequence_Data_Types[83]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ANT


.. testcode:: mwTest_Sequence_Data_Types[92]

   my_list = [5,4,3,2,7]
   print my_list   # prints '[5,4,3,2,7]'
   my_list.sort()
   print my_list   # prints '[2,3,4,5,7]'

.. testoutput:: mwTest_Sequence_Data_Types[92]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   [5, 4, 3, 2, 7]
   [2, 3, 4, 5, 7]


.. testcode:: mwTest_Sequence_Data_Types[105]

   def greater(a,b):
       if a > b:
           return -1
       elif a == b:
           return 0
       else:
           return 1
          
   l = [5,4,3,2,7]
   l.sort(greater)
   print l  #prints list in descending order

.. testoutput:: mwTest_Sequence_Data_Types[105]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   [7, 5, 4, 3, 2]


