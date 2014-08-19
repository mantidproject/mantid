.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is intended to identify detectors that are grossly over or under
counting. It reads the input workspace and identifies all histograms
with numbers of counts outside the user defined upper and lower limits.
Each spectra that fails has its spectra masked on the output workspace.
Spectra that pass the test have their data set to a positive value, 1.0.
The output workspace can be fed to :ref:`algm-MaskDetectors` to
mask the same spectra on another workspace.

ChildAlgorithms used
####################

Uses the :ref:`algm-Integration` algorithm to sum the spectra.


Usage
-----

**Example:**

.. testcode:: ExFindDetsOutsideLimits

    import numpy as np
    ws = CreateSampleWorkspace(BankPixelWidth=10,NumBanks=1)

    #create dome dead and noisy detectors
    deadDetArray=[0.0] * ws.blocksize()
    noisyDetArray= [100.0] * ws.blocksize()

    for i in range(0,ws.getNumberHistograms(),5):
        ws.setY(i,np.array(deadDetArray))
        ws.setY(i+1,np.array(noisyDetArray))

    print "With just the default LowThreshold of 0"
    (wsOut,NumberOfFailures)=FindDetectorsOutsideLimits(ws)
    print "%i spectra were outside the limits." % NumberOfFailures
    print

    print "With a High and LowThreshold, as well as restricting the XRange to consider"
    (wsOut2,NumberOfFailures)=FindDetectorsOutsideLimits(ws, HighThreshold=1000, 
        LowThreshold=0, RangeLower=200, RangeUpper=10000)
    print "%i spectra were outside the limits." % NumberOfFailures

    mtd.clear()


Output:

.. testoutput:: ExFindDetsOutsideLimits

    With just the default LowThreshold of 0
    20 spectra were outside the limits.

    With a High and LowThreshold, as well as restricting the XRange to consider
    40 spectra were outside the limits.



.. categories::
