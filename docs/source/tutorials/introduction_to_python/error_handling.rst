.. _error_handling:

==============
Error Handling 
==============

-  As we saw earlier Python can give error messages under certain
   conditions. This is known as raising an exception and Python's
   default behaviour is to halt execution. Exceptions are raised using
   the ``raise`` keyword:

.. code:: python

   # ...

   # Execution stopped:
   raise RuntimeError

-  In some cases we may be able to manage the error and still continue.
   This is known as exception handling and is achieved using
   ``try ... except`` clauses,

.. testcode:: tryExcept1

   arr = [1,2,3,4,5]

   for i in range(6):
       try:
           val = arr[i]
           print(str(val))
       except IndexError:
           print('Error: List index out of range, leaving loop')
           break

-  If an exception is raised then the code immediately jumps to the
   nearest ``except`` block. The output of the above code block is:

.. testoutput:: tryExcept1

   1
   2
   3
   4
   5
   Error: List index out of range, leaving loop

-  As with other control structures there is an extra ``else`` clause
   that can be added which will only be executed if no error was raised,

.. testcode:: tryExcept2

   arr = [1,2,3,4,5]
   value = 0
   try:
      value = arr[5]
   except IndexError:
      print('5 is not a valid array index')
   else:
      print('6th element is ' + str(value))

gives the output

.. testoutput:: tryExcept2

   5 is not a valid array index

-  With a ``try...except...else`` structure only one of the except/else
   clauses will ever be executed. In some circumstances however it is
   necessary to perform some operation, maybe a clean up, regardless of
   whether an exception was raised. This is done with a
   ``try...except...finally`` structure,

.. testcode:: tryExcept3

   value = 0
   arr = [1,2,3,4]
   element = 6
   try:
       value = arr[element]
   except IndexError:
       print(str(element) + ' is not a valid array index')
   else:
       print(str(element + 1 ) + 'th element is ' + str(value))
   finally:
       print('Entered finally clause, do cleanup ...')

gives the output

.. testoutput:: tryExcept3

   6 is not a valid array index
   Entered finally clause, do cleanup ...

-  Changing the value of the element variable between valid/invalid
   values will show that one of the except/else clauses gets executed
   and then the finally clause always gets executed.

-  It is also possible to catch exceptions of any type by leaving off
   the specific error that is to be caught. This is however not
   recommended as then it is not possible to say exactly what error
   occurred

.. testcode:: tryExcept4

   value = 0
   arr = [1,2,3,4]
   element = 6
   try:
       value = arr[element]
   except:     # Catch everything
       print("Something went wrong but I don't know what")

gives the output

.. testoutput:: tryExcept4
 
   Something went wrong but I don't know what

`Category:Tested Examples <Category:Tested_Examples>`__
