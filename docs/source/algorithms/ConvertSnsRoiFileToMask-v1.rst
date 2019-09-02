.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm reads in an old SNS reduction ROI file and converts it
into a Mantid mask workspace. It will save that mask to a Mantid mask
file.

The file format of the ROI file looks like:
::

 bank1_0_0
 bank1_0_1
 ...

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    # Run converter
    inst_name = "CNCS"
    ConvertSnsRoiFileToMask("cncs_roi.txt", inst_name, ".")

    # To test, load data and mask
    ws = Load("CNCS_7860_event.nxs")
    mask_file = inst_name + "_Mask.xml"
    mask = LoadMask(inst_name, mask_file, RefWorkspace = ws)
    MaskDetectors(ws, MaskedWorkspace=mask)

    # Check to see that only first 2 pixels are not masked
    print("Is detector 0 masked: {}".format(ws.getDetector(0).isMasked()))
    print("Is detector 1 masked: {}".format(ws.getDetector(1).isMasked()))
    print("Is detector 2 masked: {}".format(ws.getDetector(2).isMasked()))

Output:

.. testoutput:: Ex

    Is detector 0 masked: False
    Is detector 1 masked: False
    Is detector 2 masked: True

.. testcleanup:: Ex

    import os
    os.remove(os.path.join(".", mask_file))

.. categories::

.. sourcelink::
