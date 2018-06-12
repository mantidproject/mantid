.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm searches over all of the values in a workspace and if it
finds a value set to NaN (not a number), infinity, larger or smaller than the
'big'/'small' threshold given then that value and the associated error is
replaced by the user provided values.

If no value is provided for either NaNValue, InfinityValue, BigValueThreshold 
or SmallValueThreshold then the algorithm will exit with an error, as in this
case it would not be checking anything.

The algorithm can also handle event workspaces. 

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
   yArray[4] = 8e-7
   ws.setY(0,yArray)
  
   ws = ReplaceSpecialValues(ws,NaNValue=0,InfinityValue=1000,
    BigNumberThreshold=1000, BigNumberValue=1000, 
    SmallNumberThreshold=1e-6, SmallNumberValue=200)

   print("i\tBefore\tAfter")
   print("-\t------\t-----")
   for i in range(5):
       print("{}\t{}\t{}".format(i, yArray[i],ws.readY(0)[i]))
 
Output:

.. testoutput:: replaceSV
    :options: +NORMALIZE_WHITESPACE

    i       Before  After
    -       ------  -----
    0       8e+80   1000.0
    1       inf     1000.0
    2       -inf    1000.0
    3       nan     0.0
    4       8e-07   200.0
    
.. testcode:: replaceSVFloatingPointErrors

    import numpy as np
    ws = CreateSampleWorkspace(BankPixelWidth=1)

    value1 = 1.00000004
    value2 = 1.00000003
    valueDiff = value1 - value2
    
    wsYArray = np.array(ws.readY(0))
    wsYArray[0] = valueDiff
    ws.setY(0, wsYArray)
    ws = ReplaceSpecialValues(ws, SmallNumberThreshold=1e-6)
    
    print("Before\t\t After")
    print("{0:.11e}\t{1:.1f}".format(wsYArray[0], ws.readY(0)[0]))
    
Output:

.. testoutput:: replaceSVFloatingPointErrors
    :options: +NORMALIZE_WHITESPACE

    Before		  After
    9.99999993923e-09	0.0


.. categories::

.. sourcelink::
