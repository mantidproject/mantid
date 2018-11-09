.. _working_with_functions:

======================
Working With Functions 
======================

-  Functions are useful for splitting out common code that can be reused
   to perform repetitive tasks. In Python the keyword ``def`` is used to
   mark a function and as with the other control structures the block of
   code that defines the function is indented

Simple Functions
================

-  The simplest function takes no parameters and returns nothing, e.g.

.. code:: python

   def sayHello():
       print(' ----- HELLO !!! ----- ')

where now every call to ``sayHello()`` will produce the same message
format and if the code requires updates then there is only one place to
change.

Function arguments
==================

-  More useful functions will accept one or multiple arguments and
   perform some action based upon their value(s). Arguments are
   specified within the braces after the function name,

.. testcode:: function1

   def printSquare(n, verbose):
       if verbose == True:
           print( 'The square of ' + str(n) + ' is: ' + str(n*n))
       elif verbose == False:
           print(str(n*n))
       else:
           print('Invalid verbose argument passed')

   printSquare(2, True)  # Produces long string
   printSquare(3, False) # Produces short string
   printSquare(3,5)      # Produces error message

Gives the output:

.. testoutput:: function1

     The square of 2 is: 4
     9
     Invalid verbose argument passed

where we have combined functions and control structures to do something
more useful.

-  In the above function calls we specified the parameters in order but
   Python allows argument names to be specified when calling so that the
   order does not matter, e.g.

.. code:: python

   printSquare(verbose = True, n = 2)  # produces the same as
                                 # printSquare(2, True)

-  Note that explicitly specifying an argument name forces all arguments
   that follow it to also have their name specified, e.g.

.. code:: python

   def foo(A, B, C, D, E):
       # ... Do something
       return
    
   foo(1, 2, 3, 4, 5)      # Correct, no names given
   foo(1, 2, 3, D=4, E=5)  # Correct as the first 3 get assigned to the first
                       # 3 of the function and then the last two are 
                       # specified by name
   foo(C=3, 1, 2, 4, 5)   # Incorrect and will fail as a name has been
                       # specified first but then Python doesn't know
                       # where to assign the rest

This kind of calling can be useful when there is a function with many
arguments and some at the end have default values (see below). The one
that the user wishes to pick out can simple be given by name

Default Arguments
=================

-  In some situations extra function parameters maybe required for extra
   functionality but a user may want a certain default value to be
   specified so that the majority of the time the function call can be
   executed without specifying the parameter,where the second argument is 
   now optional and will be assigned the given value if the function is 
   called without it.

.. testcode:: function2

   def printSquare(n, verbose = False):
        
       if verbose == True:
           print( 'The square of ' + str(n) + ' is: ' + str(n*n))
       elif verbose == False:
           print(str(n*n))
       else:
           print('Invalid verbose argument passed')
       return

   printSquare(2)                               # Produces short message
   printSquare(2, verbose = True)  # Produces long message

Gives the output:

.. testoutput:: function2

    4
    The square of 2 is: 4


Return Values
=============

-  Most functions take in arguments, perform some processing and then
   return a value to the caller. In Python this is achieved with the
   ``return`` statement.

.. testcode:: function3

   def square(n):
       return n*n

   two_squared = square(2)
   # or print it as before
   print(square(2))

Gives the output:

.. testoutput:: function3

    4

-  Python also has the ability to return multiple values from a function
   call, something missing from many other languages. In this case the
   return values should be a comma-separated list of values and Python
   then constructs a *tuple* and returns this to the caller, e.g.

.. testcode:: function4

   def square(x,y):
       return x*x, y*y

   t = square(2,3)
   print(t)  
   # Now access the tuple with usual operations

Gives the output:

.. testoutput:: function4

    (4, 9)

-  An alternate syntax when dealing with multiple return values is to
   have Python "unwrap" the tuple into the variables directly by
   specifying the same number of variables on the left-hand side of the
   assignment as there are returned from the function, e.g.

.. testcode:: function5

   def square(x,y):
       return x*x, y*y

   xsq, ysq = square(2,3)
   print(xsq)  # Prints 4
   print(ysq)  # Prints 9  
   # Tuple has vanished!

Gives the output:

.. testoutput:: function5

    4
    9

`Category:Tested Examples <Category:Tested_Examples>`__
