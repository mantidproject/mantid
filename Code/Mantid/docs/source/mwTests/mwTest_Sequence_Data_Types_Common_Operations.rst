:orphan:

.. testcode:: mwTest_Sequence_Data_Types_Common_Operations[13]

   s = [1,2,3,4,5,6]      # Also works with all other sequence types 
   print len(s)  # prints 6
    
   test = 3 in s
   print test    # prints True
   test = 7 not in s
   print test    # prints True

.. testoutput:: mwTest_Sequence_Data_Types_Common_Operations[13]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   6
   True
   True


