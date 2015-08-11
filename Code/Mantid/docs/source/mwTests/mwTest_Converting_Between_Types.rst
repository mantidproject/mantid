:orphan:

.. testcode:: mwTest_Converting_Between_Types[1]

   x = 1/2
   print x    # Prints 0!!!

.. testoutput:: mwTest_Converting_Between_Types[1]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0


.. testcode:: mwTest_Converting_Between_Types[11]

   x = 1.0/2.0
   print x    # Prints 0.5 
   
   # or using the float function float() 
   x = 1
   y = 2
   print float(x)/float(y)   # Prints 0.5 also

.. testoutput:: mwTest_Converting_Between_Types[11]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   0.5
   0.5


