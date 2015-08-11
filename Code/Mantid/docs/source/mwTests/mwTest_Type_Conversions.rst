:orphan:

.. testcode:: mwTest_Type_Conversions[1]

   x = 'The meaning of life is ... '
   answer = 42
   y = x + str(answer)  # This converts the number 5 to a string and joins
                        # it with the first string and then assigns y to a
                        # new string containing the concatenated string


.. testcode:: mwTest_Type_Conversions[14]

   print 'The meaning of life is ... '

.. testoutput:: mwTest_Type_Conversions[14]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The meaning of life is ...


.. testcode:: mwTest_Type_Conversions[23]

   # this now means the next print statement will continue from where this
   # left the cursor
   print 'The meaning of life is ... ',

.. testoutput:: mwTest_Type_Conversions[23]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The meaning of life is ...


.. testcode:: mwTest_Type_Conversions[33]

   x = 5
   y = 6
   print "X,Y:",str(x),str(y)   # prints 'X,Y: 5 6' with a newline

.. testoutput:: mwTest_Type_Conversions[33]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   X,Y: 5 6


.. testcode:: mwTest_Type_Conversions[43]

   x = 'The meaning of life is ... '
   answer = 42
   print x + str(answer)

.. testoutput:: mwTest_Type_Conversions[43]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The meaning of life is ... 42


