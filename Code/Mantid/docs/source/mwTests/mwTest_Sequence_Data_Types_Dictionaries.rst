:orphan:

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[10]

   empty_dict = {}      # Empty dictionary
   my_lookup = {'a' : 1, 'b' : 2} # A dictionary with two keys, each
                                  # mapped to the respective value


.. testsetup:: mwTest_Sequence_Data_Types_Dictionaries[18]

   my_lookup = {'a' : 1, 'b' : 2}

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[18]

   print my_lookup['b']  # prints  2

.. testoutput:: mwTest_Sequence_Data_Types_Dictionaries[18]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   2


.. testsetup:: mwTest_Sequence_Data_Types_Dictionaries[30]

   empty_dict = {}

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[30]

   empty_dict['a']   # Results in "KeyError: 'a'"

.. testoutput:: mwTest_Sequence_Data_Types_Dictionaries[30]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
   
   KeyError: 'a'


.. testsetup:: mwTest_Sequence_Data_Types_Dictionaries[43]

   empty_dict = {}

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[43]

   empty_dict['a'] = 1


.. testsetup:: mwTest_Sequence_Data_Types_Dictionaries[51]

   my_lookup = {'a' : 1, 'b' : 2}

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[51]

   my_lookup['b'] = 3   # Replaces the value that was referenced by the key 'b' with the new value 3


.. testsetup:: mwTest_Sequence_Data_Types_Dictionaries[60]

   my_lookup = {'a' : 1, 'b' : 2}

.. testcode:: mwTest_Sequence_Data_Types_Dictionaries[60]

   del my_lookup['b']   # Removes the key/value pair with the specified key
   my_lookup.clear()   # Empties the dictionary


