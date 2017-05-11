.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------


Limitations
###########

The input workspace must be a Wokspace2D with an instrument.
The X unit of the workspace has to be 'DeltaE'
Y axis must be a Spectrum axis or it's unit has to be 'Momentum Transfer'.


Usage
-----

**Example - SaveYDA**

.. testcode:: SaveYDAExample

    # import functions to work with folders
    import os
    # import functions to work with more dimensional arrays
    import numpy as np

    # create x and y data
    dataX = np.arange(12).reshape(3, 4)
    dataY = np.arange(9).reshape(3, 3)

    # create sample workspace
    ws = CreateWorkspace(DataX=dataX, DataY=dataY, DataE=np.sqrt(dataY), NSpec=3, UnitX="DeltaE")

    # add Instrument
    LoadInstrument(ws,False,InstrumentName="TOFTOF")

    #add sample Logs
    AddSampleLog(ws,"proposal_number","3")
    AddSampleLog(ws, "proposal_title", "Proposal Title")
    AddSampleLog(ws,"experiment_team","Experiment Team name")
    AddSampleLog(ws,"temperature","200.0", LogUnit="F")
    AddSampleLog(ws,"Ei","1.0",LogUnit="meV")

    # test file name
    filename = os.path.join(config["defaultsave.directory"], "TestSaveYDA.yaml")

    # save file
    SaveYDA(ws, filename)

    with open(filename,'r') as f:
        for i in range(12):
            print f.readline(),

**Output:**

.. testoutput:: SaveYDAExample

    Meta:
        format: yaml/frida 2.0
        type: gerneric tabular data
    History:
      - Proposal number 3
      - Proposal Title
      - Experiment Team name
      - data reduced with mantid
    Coord:
        x: {name: w, unit: meV}
        y: {name: 'S(q,w)', unit: meV-1}
        z: {name: 2th, unit: deg}

.. testcleanup:: SaveYDAExample

    DeleteWorkspace("ws")
    os.remove(filename)

.. categories::

.. sourcelink::