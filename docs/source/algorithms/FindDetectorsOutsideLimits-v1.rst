.. algorithm::

.. summary::

.. relatedalgorithms::

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

    print("With just the default LowThreshold of 0")
    (wsOut,NumberOfFailures)=FindDetectorsOutsideLimits(ws)
    print("{} spectra were outside the limits.".format(NumberOfFailures))
    print("")

    print("With a High and LowThreshold, as well as restricting the range to consider")
    (wsOut2,NumberOfFailures)=FindDetectorsOutsideLimits(ws, HighThreshold=1000, 
        LowThreshold=0, RangeLower=200, RangeUpper=10000)
    print("{} spectra were outside the limits.".format(NumberOfFailures))

    mtd.clear()


Output:

.. testoutput:: ExFindDetsOutsideLimits

    With just the default LowThreshold of 0
    20 spectra were outside the limits.

    With a High and LowThreshold, as well as restricting the range to consider
    40 spectra were outside the limits.

**Example:**

.. testcode:: ExDetsOutsideLimitsPartMask

    ws = CreateSimulationWorkspace('MARI','0,1,10')
    nh = ws.getNumberHistograms()
    for ind in range(nh):
        y = ws.dataY(ind)    
        if ind>=100 and ind < 300:
            y.fill(100)
        else:
            y.fill(1)       
    
    mws1,nMasked1 = FindDetectorsOutsideLimits(ws,100)
    mws2,nMasked2 = FindDetectorsOutsideLimits(ws,100,startWorkspaceIndex = 200)

    print("****************************************")
    print("full mask ws has {0} masked detectors".format(nMasked1))
    print("part mask ws  has {0} masked detectors".format(nMasked2))
    print("****************************************")
    selected_spec = [99,100,199,200,299,300]
    for spec in selected_spec:
        print("full mask ws Spec N{0} is masked: {1}".format(spec,mws1.readY(spec)[0]>0.5))
        print("part mask ws Spec N{0} is masked: {1}".format(spec,mws2.readY(spec)[0]>0.5) )
    print("****************************************")

Output:

.. testoutput:: ExDetsOutsideLimitsPartMask

    ****************************************
    full mask ws has 200 masked detectors
    part mask ws  has 100 masked detectors
    ****************************************
    full mask ws Spec N99 is masked: False
    part mask ws Spec N99 is masked: False
    full mask ws Spec N100 is masked: True
    part mask ws Spec N100 is masked: False
    full mask ws Spec N199 is masked: True
    part mask ws Spec N199 is masked: False
    full mask ws Spec N200 is masked: True
    part mask ws Spec N200 is masked: True
    full mask ws Spec N299 is masked: True
    part mask ws Spec N299 is masked: True
    full mask ws Spec N300 is masked: False
    part mask ws Spec N300 is masked: False
    ****************************************

.. categories::

.. sourcelink::
