.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm identifies tubes, which become saturated due to hign 
neutron flux (e.g. from inense bragg peaks) affecting these tubes
so that tubes are not counting neutrons but show constant high 
counts over the tube and generates mask workspace to exclude such tubes.

The effect occurs due some old data acsqusition electronics and 
observed as homogeneous high counts reading symmetric over the whole 
tube length.

The diagnostic test attempts to find all tubes within the instrument
attached to the workspace. If successful, each tube is tested for
saturation above the level defined by the production of 'MaxTubeFramerate' 
property by 'goodfrm' log value, retrieved from the workspace.

If any pixel, excluding the number of central pixels specified by 
'NIgnoredCentralPixels' property, is counting above this threshold 
then the entire tube is masked.

The image below shows "over-corrected" image, where used 'MaxTubeFramerate'
set up too low. Number of tubes with high neutron flux on them become masked.

.. image:: /images/BleedingCorrections.png

Restrictions on the input workspace
###################################

-  The workspace must contain goodfrm log with value ideally
specifying the good frames arrival rate. Any value proportional to this 
rate would work after appropriate adjasment to 'MaxTubeFramerate' value.


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
