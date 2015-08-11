:orphan:

.. testsetup:: mwTest_Control_Structures[25]

   x = 4

.. testcode:: mwTest_Control_Structures[25]

   if x == 5:
       print 'x has the value 5'
   else:
       print 'x does not equal 5'  
   
   # The printed statement will differ depending on the value of x

.. testoutput:: mwTest_Control_Structures[25]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   x does not equal 5


.. testsetup:: mwTest_Control_Structures[41]

   x = 4

.. testcode:: mwTest_Control_Structures[41]

   if x > 0 and x < 5:
       print 'x is between 0 and 5 (not inclusive)' 
   else:
       print 'x is outside the range 0->5'

.. testoutput:: mwTest_Control_Structures[41]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   x is between 0 and 5 (not inclusive)


.. testsetup:: mwTest_Control_Structures[55]

   x = 5

.. testcode:: mwTest_Control_Structures[55]

   if x == 5:
       print 'In x = 5 routine'
      print 'Doing correct thing'  # Results in error "IndentationError:
                                   # unindent does not match any outer
                                   # indentation level"
   else:
       print 'Everything else'

.. testoutput:: mwTest_Control_Structures[55]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
   
   IndentationError: unindent does not match any outer indentation level


.. testsetup:: mwTest_Control_Structures[76]

   x = 3

.. testcode:: mwTest_Control_Structures[76]

   if x == 1:
       print 'Running scenario 1'
   elif x == 2:
       print 'Running scenario 2'
   elif x == 3:
       print 'Running scenario 3'
   else:
       print 'Unrecognized option'

.. testoutput:: mwTest_Control_Structures[76]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Running scenario 3


.. testsetup:: mwTest_Control_Structures[94]

   x = 2

.. testcode:: mwTest_Control_Structures[94]

   if x == 1 or x == 2:
       print 'Running scenario first range'

.. testoutput:: mwTest_Control_Structures[94]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Running scenario first range


