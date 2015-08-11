:orphan:

.. testcode:: mwTest_Basic_Language_Principles[26]

   import numpy
   # Here x is initialized to 5 and Python then treats this as an integer
   x = 5
   # It can be incremented and have all of the expected operations applied to it
   x += 1
   
   # Later on it can be used for something else
   # x is a string and adding a number produces an error
   x = "a string"  
   x + 5
   
   #will give you an error
   #Traceback (most recent call last):
   # File "<stdin>", line 1, in <module>
   #TypeError: cannot concatenate 'str' and 'int' objects

.. testoutput:: mwTest_Basic_Language_Principles[26]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
    File "<stdin>", line 1, in <module>
   TypeError: cannot concatenate 'str' and 'int' objects


