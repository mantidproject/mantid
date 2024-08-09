.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

.. |sym| replace:: \+

.. |verb| replace:: added

.. |prep| replace:: to

.. include:: BinaryOperation.txt

EventWorkspace note
###################

For :ref:`EventWorkspaces <EventWorkspace>`, the event lists at each
workspace index are concatenated to create the output event list at the
same workspace index. Note that in some (rare :sup:`\*`) cases, these
event lists might be from different detectors; this is not checked
against and the event lists will be concatenated anyway. This may or may
not be your desired behavior. If you wish to merge different
EventWorkspaces while matching their detectors together, use the
:ref:`algm-MergeRuns` algorithm.

:sup:`\*` This could happen, for example, if the workspace operands have
not both been processed in an identical fashion and the detectors have
somehow been grouped differently.

Usage
-----

**Example - Plus as an Algorithm**

.. testcode:: ExPlusAsAlg

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)
    ws2 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws = Plus(ws1, ws2)

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExPlusAsAlg

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 2.  3.  4.  5.  6.  7.  8.  9. 10.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Plus as an operator**

.. testcode:: ExPlusAsOpperand

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)
    ws2 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws = ws1 + ws2

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExPlusAsOpperand

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 2.  3.  4.  5.  6.  7.  8.  9. 10.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Addition using in-place operator**

.. testcode:: ExPlusInPlace

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    dataX2 = [0,1,2,3,4,5,6,7,8,9] #X-values must be identical
    dataY2 = [2,2,2,2,2,2,2,2,2]
    dataE2 = [3,3,3,3,3,3,3,3,3]
    ws = CreateWorkspace(dataX1, dataY1, dataE1)
    ws1 = CreateWorkspace(dataX2, dataY2, dataE2)

    # perform the algorithm
    ws += ws1

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExPlusInPlace

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 2.  3.  4.  5.  6.  7.  8.  9. 10.]
    The updated Error values are: [3.         3.16227766 3.60555128 4.24264069 5.         5.83095189
     6.70820393 7.61577311 8.54400375]

**Example - Add a scalar**

.. testcode:: ExPlusWithSingleVal

    # create histogram workspaces
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [0,1,2,3,4,5,6,7,8] # or use dataE1=range(0,9)
    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    # perform the algorithm
    ws = ws1 + 2.5

    print("The X values are: " + str(ws.readX(0)))
    print("The Y values are: " + str(ws.readY(0)))
    print("The updated Error values are: " + str(ws.readE(0)))

Output:

.. testoutput:: ExPlusWithSingleVal

    The X values are: [0. 1. 2. 3. 4. 5. 6. 7. 8. 9.]
    The Y values are: [ 2.5  3.5  4.5  5.5  6.5  7.5  8.5  9.5 10.5]
    The updated Error values are: [0. 1. 2. 3. 4. 5. 6. 7. 8.]

.. categories::

.. sourcelink::
