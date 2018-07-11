.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Clones the input :ref:`Matrix Workspaces <MatrixWorkspace>` and orders the
x-axis in an ascending or descending fashion. Ensures that the y-axis and error data as well as optional Dx data
are sorted in a consistent way with the x-axis.

This algorithm is for use with workspaces of all sizes loaded into mantid (with varying speeds). It is
particularly suitable for reformatting workspaces loaded via
:ref:`LoadAscii <algm-LoadAscii>`. Input workspaces must be point data.

Usage
-----
.. testcode:: SortXAxis

    #Import requirements
    import numpy as np

    # Create Sample Workspace and get a spectrum to view
    ws = CreateSampleWorkspace()

    # Print out the "Unordered X Axis"
    print("Unordered Print")
    print(ws.readX(0))

    # Sort the X Axis in a Descending fashion
    ws = SortXAxis(InputWorkspace='ws', Ordering='Descending', Version=2)
    print("In order print: Descending")
    print(ws.readX(0))

    # Sort the X Axis in a Ascending fashion
    ws = SortXAxis(InputWorkspace='ws', Ordering='Ascending', Version=2)
    print("In order print: Ascending")
    print(ws.readX(0))

.. testcleanup:: SortXAxis

    DeleteWorkspace(ws)

Output:

.. testoutput:: SortXAxis

       Unordered Print
    [     0.    200.    400.    600.    800.   1000.   1200.   1400.   1600.
       1800.   2000.   2200.   2400.   2600.   2800.   3000.   3200.   3400.
       3600.   3800.   4000.   4200.   4400.   4600.   4800.   5000.   5200.
       5400.   5600.   5800.   6000.   6200.   6400.   6600.   6800.   7000.
       7200.   7400.   7600.   7800.   8000.   8200.   8400.   8600.   8800.
       9000.   9200.   9400.   9600.   9800.  10000.  10200.  10400.  10600.
      10800.  11000.  11200.  11400.  11600.  11800.  12000.  12200.  12400.
      12600.  12800.  13000.  13200.  13400.  13600.  13800.  14000.  14200.
      14400.  14600.  14800.  15000.  15200.  15400.  15600.  15800.  16000.
      16200.  16400.  16600.  16800.  17000.  17200.  17400.  17600.  17800.
      18000.  18200.  18400.  18600.  18800.  19000.  19200.  19400.  19600.
      19800.  20000.]
    In order print: Descending
    [ 20000.  19800.  19600.  19400.  19200.  19000.  18800.  18600.  18400.
      18200.  18000.  17800.  17600.  17400.  17200.  17000.  16800.  16600.
      16400.  16200.  16000.  15800.  15600.  15400.  15200.  15000.  14800.
      14600.  14400.  14200.  14000.  13800.  13600.  13400.  13200.  13000.
      12800.  12600.  12400.  12200.  12000.  11800.  11600.  11400.  11200.
      11000.  10800.  10600.  10400.  10200.  10000.   9800.   9600.   9400.
       9200.   9000.   8800.   8600.   8400.   8200.   8000.   7800.   7600.
       7400.   7200.   7000.   6800.   6600.   6400.   6200.   6000.   5800.
       5600.   5400.   5200.   5000.   4800.   4600.   4400.   4200.   4000.
       3800.   3600.   3400.   3200.   3000.   2800.   2600.   2400.   2200.
       2000.   1800.   1600.   1400.   1200.   1000.    800.    600.    400.
        200.      0.]
    In order print: Ascending
    [     0.    200.    400.    600.    800.   1000.   1200.   1400.   1600.
       1800.   2000.   2200.   2400.   2600.   2800.   3000.   3200.   3400.
       3600.   3800.   4000.   4200.   4400.   4600.   4800.   5000.   5200.
       5400.   5600.   5800.   6000.   6200.   6400.   6600.   6800.   7000.
       7200.   7400.   7600.   7800.   8000.   8200.   8400.   8600.   8800.
       9000.   9200.   9400.   9600.   9800.  10000.  10200.  10400.  10600.
      10800.  11000.  11200.  11400.  11600.  11800.  12000.  12200.  12400.
      12600.  12800.  13000.  13200.  13400.  13600.  13800.  14000.  14200.
      14400.  14600.  14800.  15000.  15200.  15400.  15600.  15800.  16000.
      16200.  16400.  16600.  16800.  17000.  17200.  17400.  17600.  17800.
      18000.  18200.  18400.  18600.  18800.  19000.  19200.  19400.  19600.
      19800.  20000.]   

.. categories::

.. sourcelink::
