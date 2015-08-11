:orphan:

.. testcode:: mwTest_Sequence_Data_Types_Tuples[4]

   lottery_numbers = (1,2,3,4,5,6)


.. testsetup:: mwTest_Sequence_Data_Types_Tuples[10]

   lottery_numbers = (1,2,3,4,5,6)

.. testcode:: mwTest_Sequence_Data_Types_Tuples[10]

   print lottery_numbers[0]     # prints 1
   print lottery_numbers[1:3]   # prints (2,3)
   # Assignment not allowed
   lottery_numbers[3] = 42      # gives error "TypeError: 'tuple' object does not support item assignment"

.. testoutput:: mwTest_Sequence_Data_Types_Tuples[10]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
   
   TypeError: 'tuple' object does not support item assignment


