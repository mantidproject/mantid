.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a workspace to a DAVE grp file. A description of the DAVE grouped
data format can be found
`here <http://www.ncnr.nist.gov/dave/documentation/ascii_help.pdf>`_.

Usage
-----

The DAVE grouped ASCII file is normally used to support S(Q, E) workspaces. So,
a contrived example will be created.

.. include:: ../usagedata-note.txt

.. testcode:: Ex

    ws = Load("CNCS_7860_event.nxs")
    ws = ConvertUnits(ws, Target="DeltaE", EMode="Direct", EFixed=3.0)
    ws = Rebin(ws, Params=[-3,0.01,3], PreserveEvents=False)
    ws = SofQW(ws, QAxisBinning=[0.2,0.2,3], EMode="Direct", EFixed=3.0)
    print("Workspace size = ( {} , {} )".format(ws.getNumberHistograms(), ws.blocksize()))
    import os
    savefile = os.path.join(config["default.savedirectory"], "CNCS_7860_sqw.grp")
    SaveDaveGrp(ws, Filename=savefile)
    print("File created: {}".format(os.path.exists(savefile)))
    ifile = open(savefile, 'r')
    lines = ifile.readlines()
    ifile.close()
    # Number of lines = header(4) + Q axis spec(1 + 14) + E axis spec(1 + 600)
    #                   + data(14 * 601)
    print("Number of lines = {}".format(len(lines)))

Output:

.. testoutput:: Ex

    Workspace size = ( 14 , 600 )
    File created: True
    Number of lines = 9034

.. testcleanup:: Ex

    import os
    os.remove(savefile)

.. categories::

.. sourcelink::
