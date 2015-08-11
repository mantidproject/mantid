:orphan:

.. testcode:: mwTest_Working_With_Functions[6]

   def sayHello():
       print ' ----- HELLO !!! ----- '


.. testcode:: mwTest_Working_With_Functions[16]

   def printSquare(n, verbose):
       if verbose == True:
           print 'The square of ' + str(n) + ' is: ' + str(n*n)
       elif verbose == False:
           print str(n*n)
       else:
           print 'Invalid verbose argument passed'
   
   printSquare(2, True)  # Produces long string
   printSquare(3, False) # Produces short string
   printSquare(3,5)      # Produces error message

.. testoutput:: mwTest_Working_With_Functions[16]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The square of  2 is: 4
   9
   Invalid verbose argument passed


.. testsetup:: mwTest_Working_With_Functions[39]

   def printSquare(n, verbose):
       if verbose == True:
           print 'The square of ' + str(n) + ' is: ' + str(n*n)
       elif verbose == False:
           print str(n*n)
       else:
           print 'Invalid verbose argument passed'

.. testcode:: mwTest_Working_With_Functions[39]

   printSquare(verbose = True, n = 2)  # produces the same as
                                 # printSquare(2, True)

.. testoutput:: mwTest_Working_With_Functions[39]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   The square of  2 is: 4


.. testcode:: mwTest_Working_With_Functions[57]

   def foo(A,B,C,D,E):
       # ... Do something
       return
    
   foo(1,2,3,4,5)      # Correct, no names given
   foo(1,2,3,D=4,E=5)  # Correct as the first 3 get assigned to the first
                       # 3 of the function and then the last two are 
                       # specified by name
   foo(C=3,1, 2,4,5)   # Incorrect and will fail as a name has been
                       # specified first but then Python doesn't know
                       # where to assign the rest

.. testoutput:: mwTest_Working_With_Functions[57]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Traceback (most recent call last):
   
   SyntaxError: non-keyword arg after keyword arg


.. testcode:: mwTest_Working_With_Functions[81]

   def printSquare(n, verbose = False):
       # definition same as above
       return
   
   printSquare(2)                               # Produces short message
   printSquare(2, verbose = True)  # Produces long message


