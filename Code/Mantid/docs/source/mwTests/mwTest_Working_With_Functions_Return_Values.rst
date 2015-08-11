:orphan:

.. testcode:: mwTest_Working_With_Functions_Return_Values[1]

   def square(n):
       return n*n
   
   two_squared = square(2)
   # or print it as before
   print square(2)

.. testoutput:: mwTest_Working_With_Functions_Return_Values[1]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   4


.. testcode:: mwTest_Working_With_Functions_Return_Values[15]

   def square(x,y):
       return x*x, y*y
   
   t = square(2,3)
   print t  # Produces (4,9)
   # Now access the tuple with usual operations

.. testoutput:: mwTest_Working_With_Functions_Return_Values[15]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   (4, 9)


.. testcode:: mwTest_Working_With_Functions_Return_Values[28]

   def square(x,y):
       return x*x, y*y
   
   xsq, ysq = square(2,3)
   print xsq  # Prints 4
   print ysq  # Prints 9  
   # Tuple has vanished!

.. testoutput:: mwTest_Working_With_Functions_Return_Values[28]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   4
   9


