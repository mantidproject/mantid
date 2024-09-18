.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a workspace with momentum transfer units into a file adhering to the NXcanSAS format specified by NXcanSAS Data
Formats Working Group `schema <http://cansas-org.github.io/NXcanSAS/classes/contributed_definitions/NXcanSAS.html>`__.

1D or 2D workspaces may be saved.

If the input workspace is 2D then the vertical axis needs to be a numeric axis in momentum transfer units. The created
file can be reloaded using the :ref:`algm-LoadNXcanSAS` algorithm.

In addition, it is possible to save the transmission workspaces obtained from a reduction.


Usage
-----

**Example**

.. testcode:: SaveNXcanSAS

    import os

    # Create a example workspace with units of momentum transfer
    ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
    ws = ConvertUnits(ws,Target="MomentumTransfer")
    LoadInstrument(ws,False,InstrumentName="SANS2D")

    # Save the file
    file_name = "test_file_for_nxcansas"
    SaveNXcanSAS(ws,file_name)

    # Load the file back
    ws_loaded = LoadNXcanSAS(file_name)

    #remove the file we created
    alg = ws_loaded.getHistory().lastAlgorithm()
    filePath = alg.getPropertyValue("Filename")
    os.remove(filePath)

.. categories::

.. sourcelink::
