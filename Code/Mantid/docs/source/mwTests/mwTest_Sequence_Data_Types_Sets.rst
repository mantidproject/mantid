:orphan:

.. testcode:: mwTest_Sequence_Data_Types_Sets[7]

   values = set([1,1,3])
   print values  # prints 'set('[1,3])'

.. testoutput:: mwTest_Sequence_Data_Types_Sets[7]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   set([1, 3])


.. testsetup:: mwTest_Sequence_Data_Types_Sets[17]

   values = set([1,1,3])

.. testcode:: mwTest_Sequence_Data_Types_Sets[17]

   values.add(4)
   values.remove(1)
   print values  #  prints 'set('[3,4])'

.. testoutput:: mwTest_Sequence_Data_Types_Sets[17]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   set([3, 4])


