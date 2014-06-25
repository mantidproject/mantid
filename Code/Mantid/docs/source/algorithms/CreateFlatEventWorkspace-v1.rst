.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This creates a new event workspace from an existing one by sampling an area of the 
source workspaces background and then using this to create a new workspace that covers 
the same data range as the input workspace.

Usage
-----

**Example: Extract the background into another workspace**

.. testcode:: Extractbackground
    
    ws = CreateSampleWorkspace("Event","Multiple Peaks")
    wsOut=CreateFlatEventWorkspace(ws,RangeStart=15000,RangeEnd=18000)

    #to compare we need to match the bins
    wsOut=RebinToWorkspace(wsOut,ws,PreserveEvents=True)

    print "The values for every 10th bin."
    print "bin\tws\twsOut"
    for i in range (0,ws.blocksize(),10):
        print "%i\t%.2f\t%.2f" % (i,ws.readY(0)[i],wsOut.readY(0)[i])

Output:

.. testoutput:: Extractbackground
    :options: +NORMALIZE_WHITESPACE

    The values for every 10th bin.
    bin     ws      wsOut
    0       16.00   16.00
    10      16.00   16.00
    20      16.00   16.00
    30      572.00  16.00
    40      16.00   16.00
    50      16.00   16.00
    60      461.00  16.00
    70      16.00   16.00
    80      16.00   16.00
    90      16.00   16.00



.. categories::
