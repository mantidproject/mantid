.. _type_conversions:

================
Type Conversions 
================

-  In many circumstances we want to be able to perform operations on
   differing types, as in the earlier example where we tried to add the
   number '5' to a string variable. In this situation what we really
   meant was take a string representation of the number 5 and join that
   together with our first string. This is called a type conversion and
   for the string case it is achieved with the ``str()``, e.g.

.. code:: python

   x = 'The meaning of life is ... '
   answer = 42
   y = x + str(answer)  # This converts the number 42 to a string and joins
                        # it with the first string and then assigns y to a
                        # new string containing the concatenated string 

Printing
========

-  The most useful situation for this function is when printing output
   to a screen. Printing is achieved with the ``print`` command that
   expects a string, e.g.
-  in Python 2, you do not need to provide the brackets around the
   arguments of the print function, but we have included them here, so
   these examples will work with either python 2 or 3.

.. code:: python

   print('The meaning of life is ... ')

-  By default the ``print`` command outputs a new line to the screen. To
   suppress this behaviour add a comma after the string:

.. code:: python

   # this now means the next print statement will continue from where this
   # left the cursor
   # In python 2 (in this case you cannot include the brackets)
   print 'The meaning of life is ... ',
   # the equivalent in python 3 is
   #print('The meaning of life is ... ', end=' ')

-  The comma can also be used to print several things to the screen on
   one line separated by a space:

.. code:: python

   x = 5
   y = 6
   # Python 2
   print "X,Y:",str(x),str(y)   # prints 'X,Y: 5 6' with a newline
   #print("X,Y:",str(x),str(y))   # prints '('X,Y:', '5', '6')' with a newline
   # Python 3
   #print ("X,Y:",str(x),str(y))  # prints 'X,Y: 5 6' with a newline

-  As above printing other types is then simple with the ``str()``
   function:

.. testcode:: types1

   x = 'The meaning of life is ... '
   answer = 42
   print(x + str(answer))

Gives the output:

.. testoutput:: types1

    The meaning of life is ... 42

-  If you want to avoid wrapping variables in str, you can use the
   format statement to insert values into a template string:

.. testcode:: types2

   answer = 42
   print('The meaning of life is ... {}'.format(answer))

Gives the output:

.. testoutput:: types2

    The meaning of life is ... 42

Converting Between Types
========================

-  Type conversions are not only important for converting to strings but
   are sometimes necessary to achieve expected answers, e.g.

.. code:: python

   x = 1/2
   print(x)    # Prints 0!!! in Python 2 and 0.5 in Python 3

-  In this case we have asked Python to take two integers (1,2) and then
   divide them and assign the result to ``x``. The result is another
   integer which in this case is the integer part of the real number
   0.5. If, as in this case, the real number is required then we must
   ask Python to use floating point numbers instead of integers. This
   can be achieved in two ways:

.. testcode:: types3

   x = 1.0/2.0
   print(x)    

   # or using the float function float() 
   x = 1
   y = 2
   print(float(x)/float(y))   

Gives the output:

.. testoutput:: types3

    0.5
    0.5

-  The type conversion functions for the 4 basic types are:

+---------+----------+-------------------+
| Type    | Function | Example           |
+---------+----------+-------------------+
| integer | int()    | int(3.14159) => 3 |
+---------+----------+-------------------+
| float   | float()  | float(5) => 5.0   |
+---------+----------+-------------------+
| bool    | bool()   | bool(5) => True   |
+---------+----------+-------------------+
| string  | str()    | str(5) => '5'     |
+---------+----------+-------------------+
|         |          |                   |
+---------+----------+-------------------+

-  If a type cannot be converted then a 'ValueError' occurs (see error
   handling section).

`Category:Tested Examples <Category:Tested_Examples>`__ `Category:Tested
Examples <Category:Tested_Examples>`__
