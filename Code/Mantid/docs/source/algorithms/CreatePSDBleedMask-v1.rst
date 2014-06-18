.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The diagnostic test attempts to find all tubes within the instrument
attached to the workspace. If successful, each tube is tested for
saturation above the level defined by the 'MaxTubeFramerate' property.
If any pixel, not including those marked to be ignored around the
equatorial region, are counting above this threshold then the entire
tube is masked.

Restrictions on the input workspace
###################################

-  The workspace must contain either raw counts or counts/us.

Usage
-----

**Example:**

.. testcode:: ExPSDMask

    import numpy as np

    ws=CreateSampleWorkspace()
    AddSampleLog(ws,"goodfrm","10","Number")
    noisyY =  np.array(ws.readY(0))
    noisyY[0]=1e20
    ws.setY(50,noisyY)
    (wsOut, numFailures) = CreatePSDBleedMask(ws,MaxTubeFramerate=10, NIgnoredCentralPixels=2)

    print "%i spectra have been masked in wsOut" % numFailures


Output:

.. testoutput:: ExPSDMask

    10 spectra have been masked in wsOut



.. categories::
