.. _looping:

=======
Looping 
=======

-  Executing a given block of code a number of times is achieved using
   either the ``for`` or ``while`` statements

For ... else
============

-  The syntax to loop with a ``for`` statement is

.. code:: python

   for ''target'' in sequence:
       ''block-of-code-to-execute''

where *target* gets the value of the current number in *sequence*.

-  A sequence from the loop can be a predefined sequence that has been
   constructed previously within the code or the
   ``range(start, end, step)`` function can be used to generate a
   sequence of numbers from *start* to *end-1* in steps of *step*. The
   default step value is *1*. E.g.

.. testcode:: loop1

   for i in range(0,10):
       print(i)

gives the output

.. testoutput:: loop1

   0
   1
   2
   3
   4
   5
   6
   7
   8
   9

.. testcode:: loop2

   for i in range(0,10,2):
       print(i)

gives the output

.. testoutput:: loop2

   0
   2
   4
   6
   8

-  Extra loop control is provided by the ``break`` and ``continue``
   statements. ``break`` causes the loop to terminate at that point
   without executing any other code in the block,

.. testcode:: loop3

   nums = [1,2,3,-1,5,6]
   list_ok = True
   for i in nums:
       if i < 0:
           list_ok = False
           break

   if list_ok == False:
       print('The list contains a negative number')

gives the output

.. testoutput:: loop3

   The list contains a negative number

-  ``continue`` causes execution to immediately jump to the next
   iteration of the loop,

.. testcode:: loop4

   nums =  [1,2,3,-1,5,6]
   pos_sum = 0
   for i in nums:
       if i < 0:
           continue
       pos_sum += i     # compound assignment means pos_sum = pos_sum + i

   print('Sum of positive numbers is ' + str(pos_sum))

gives the output

.. testoutput:: loop4

   Sum of positive numbers is 17

-  An optional ``else`` clause can be added after the loop that will
   only get executed if the whole loop executes successfully,

.. testcode:: loop5

   for i in range(0,10):
       print(i)
   else:
       print('done')     # Prints numbers 0-9 and the 'done'

gives the output

.. testoutput:: loop5

   0
   1
   2
   3
   4
   5
   6
   7
   8
   9
   done

.. testcode:: loop5

   for i in range(0,10):
       if i == 5:
           break
       print(i)
   else:
       print('done')     # Prints numbers 0-4

gives the output

.. testoutput:: loop5

   0
   1
   2
   3
   4

While
=====

-  While is another looping statement that simple executes until a given
   statement is False,

.. testcode:: loop6

   sum = 0
   while sum < 10:
       sum += 1   # ALWAYS remember to update the loop test or it will
                          # run forever!! 

   print(sum)      # Gives value 10

gives the output

.. testoutput:: loop6

   10

-  The ``while`` loop also supports the else syntax in the same manner
   as the ``for`` loop

`Category:Tested Examples <Category:Tested_Examples>`__
