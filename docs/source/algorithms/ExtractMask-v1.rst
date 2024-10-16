.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The masking from the InputWorkspace property is extracted by creating a
new MatrixWorkspace with a single X bin where:

-  0 = masked;
-  1 = unmasked.

The spectra containing 0 are also marked as masked and the instrument
link is preserved so that the instrument view functions correctly.

A list of masked detector IDs is also output. Note this contains the detector IDs which
are masked rather than the index or spectrum number.

Using Instrument donor
----------------------

The ``InstrumentDonor`` parameter can be used to specify a workspace from which the ``MaskWorkspace`` creation instrument is based on. The masking will still be copied from the ``InputWorkspace`` but will be applied by matching the detector IDs from the ``InstrumentDonor`` workspace.

You can specify the ``InstrumentDonor`` without the ``InputWorkspace``. In this case, the output ``MaskWorkspace`` will have all spectra unmasked.

Usage
-----

**Example**

.. testcode:: ExExtractMask

    #create a workspace with a 3*3 pixel detector
    bankPixelWidth = 3
    ws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=bankPixelWidth)

    #Mask out every other detector
    MaskDetectors(ws,WorkspaceIndexList=range(0,bankPixelWidth*bankPixelWidth,2))

    wsMask, maskList = ExtractMask(ws)

    #This mask can then be applied to another workspace
    ws2 = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=bankPixelWidth)
    MaskDetectors(ws2,MaskedWorkspace="wsMask")

    print("Masked Detectors")
    print("n ws    ws2")
    for i in range (ws.getNumberHistograms()):
        print("%i %-5s %s" % (i, ws.getDetector(i).isMasked(), ws2.getDetector(i).isMasked()))

    print("\nMasked detector IDs")
    print(maskList)

Output:

.. testoutput:: ExExtractMask

    Masked Detectors
    n ws    ws2
    0 True  True
    1 False False
    2 True  True
    3 False False
    4 True  True
    5 False False
    6 True  True
    7 False False
    8 True  True

    Masked detector IDs
    [ 9 11 13 15 17]

.. categories::

.. sourcelink::
