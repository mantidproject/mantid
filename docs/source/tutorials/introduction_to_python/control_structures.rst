.. _control_structures:

==================
Control Structures 
==================

-  As in any programming language there are mechanisms to control
   program flow: ``if ... else``, ``for ...``, ``while``

Comparisons testing
===================

-  Control flow requires knowing how to compare values, for instance
   does one value equal another. In Python there are 6 operators that
   deal with comparisons:

   -  ``==`` Tests for equality of two values, e.g. ``x == 2``
   -  ``!=`` Tests for inequality of two values, e.g. ``x != 2``
   -  ``<`` Tests if lhs is less than rhs, e.g. ``x < 2``
   -  ``>`` Tests if lhs is greater than rhs, e.g. ``x > 2``
   -  ``<=`` Tests if lhs is less than or equal to rhs, e.g. ``x <= 2``
   -  ``>=`` Tests if lhs is greater than or equal rhs, e.g. ``x >= 2``

| 

Control blocks
==============

-  In Python bodies within control blocks are defined by indentation:
   spaces or tabs. Each level of indentation defines a separate control
   block. Tabs and spaces should never be mixed and each block must have
   the same indentation level.

If else
=======

-  The simplest control structure runs one of two different blocks of
   code depending on the value of a test,

.. code:: python

   if x == 5:
       print('x has the value 5')
   else:
       print('x does not equal 5')

   # The printed statement will differ depending on the value of x

-  To test for ranges combine test with the ``and`` keyword

.. code:: python

   if x > 0 and x < 5:
       print('x is between 0 and 5 (not inclusive)')
   else:
       print('x is outside the range 0->5')

-  Here we show an example of incorrect indentation and the subsequent
   error,

.. code:: python

   if x == 5:
       print('In x = 5 routine')
      print ('Doing correct thing')  # Results in error "IndentationError:
                                   # unindent does not match any outer
                                   # indentation level"
   else:
       print('Everything else')

If ... elif ... else
====================

-  For situations with more than 2 possible outcomes there is an
   enhanced version of ``if ... else`` using the keyword ``elif`` to add
   additional blocks, e.g.

.. code:: python

   if x == 1:
       print('Running scenario 1')
   elif x == 2:
       print('Running scenario 2')
   elif x == 3:
       print('Running scenario 3')
   else:
       print('Unrecognized option')

-  Tests can also be combined with the **not** to negate the test or
   with the **or** keyword to test one of two values.

.. code:: python

   if x == 1 or x == 2:
       print('Running scenario first range')

.. raw:: mediawiki

   {{SlideNavigationLinks|Sequence_Data_Types|Introduction_To_Python|Control_Structures:_Looping}}

`Category:Tested Examples <Category:Tested_Examples>`__
