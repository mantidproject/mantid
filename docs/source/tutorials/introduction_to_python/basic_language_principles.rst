.. _basic_language_principles:

=========================
Basic Language Principles 
=========================

-  Python is an interpreted language meaning there is no explicit
   compilation step.

-  The code is simply executed "as-is". This, coupled with the fact that
   Python has a simple and easy-to-read syntax means it is an excellent
   choice for a scripting language.

-  It includes all of the features that one expects from a programming
   language such as basic numerical types, a boolean, a string type and
   support for various operations upon them. The table below summarises
   how to use them in Python:

| 

+---------+------------+------------------------------+
| Type    | Example    | Python code                  |
+---------+------------+------------------------------+
| integer | 5          | x = 5                        |
+---------+------------+------------------------------+
| float   | 5.0        | x = 5.0                      |
+---------+------------+------------------------------+
| boolean | True/False | x = True                     |
+---------+------------+------------------------------+
| string  | 'python'   | x = 'python' or x = "python" |
+---------+------------+------------------------------+

-  The operations are supported as long as it makes sense for that type,
   e.g. there is no string division but ``+`` just means join the two
   strings together.

-  Variable assignment is simple than in other languages as you do not
   have to declare the type and moreover it can be changed during
   execution, e.g.

.. code:: python

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

-  Comments are signified by the ``#`` symbol.

-  Errors are signified by things known as **Exceptions**. In the above
   example a typical error message is shown which says that an exception
   of type ``TypeError`` occurred and the program needed to terminate
   (more on handling errors later).

.. raw:: mediawiki

   {{StartNavigationLinks|Introduction_To_Python|Type_Conversions}}

`Category:Tested Examples <Category:Tested_Examples>`__
