.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm searches over all of the values in a workspace and if it
finds a value set to NaN (not a number), infinity or larger than the
'big' threshold given then that value and the associated error is
replaced by the user provided values.

If no value is provided for either NaNValue, InfinityValue or
BigValueThreshold then the algorithm will exit with an error, as in this
case it would not be checking anything.

Algorithm is now event aware.

Usage
-----

**Example**  

.. testcode:: replaceSV

   import numpy as np
   ws = CreateSampleWorkspace(BankPixelWidth=1)

   yArray = np.array(ws.readY(0))
   yArray[0] = 8e80
   yArray[1] = float("inf")
   yArray[2] = float("-inf")
   yArray[3] = float("NaN")
   ws.setY(0,yArray)
  
   ws = ReplaceSpecialValues(ws,NaNValue=0,InfinityValue=1000,
    BigNumberThreshold=1000, BigNumberValue=1000)

   print "i\tBefore\tAfter"   
   print "-\t------\t-----"
   for i in range(4):
       print "%i\t%s\t%s" % (i, yArray[i],ws.readY(0)[i])     
 
Output:

.. testoutput:: replaceSV
    :options: +NORMALIZE_WHITESPACE

    i       Before  After
    -       ------  -----
    0       8e+80   1000.0
    1       inf     1000.0
    2       -inf    1000.0
    3       nan     0.0


.. categories::
