.. algorithm::

.. summary::

.. relatedalgorithms::

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

    print("The values for every 10th bin.")
    print("bin\tws\twsOut")
    for i in range (0,ws.blocksize(),10):
        print("{}\t{:.2f}\t{:.2f}".format(i,ws.readY(0)[i],wsOut.readY(0)[i]))

Output:

.. testoutput:: Extractbackground
    :options: +NORMALIZE_WHITESPACE

    The values for every 10th bin.
    bin     ws      wsOut
    0       6.00    6.00
    10      6.00    6.00
    20      6.00    6.00
    30      214.00  6.00
    40      6.00   6.00
    50      6.00   6.00
    60      172.00  6.00
    70      6.00   6.00
    80      6.00   6.00
    90      6.00   6.00



.. categories::

.. sourcelink::
