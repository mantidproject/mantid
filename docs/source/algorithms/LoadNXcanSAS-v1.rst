.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads the given file, which should be in the NXcanSAS format specified
by NXcanSAS Data Formats Working Group schema
http://cansas-org.github.io/NXcanSAS/classes/contributed_definitions/NXcanSAS.html and
creates an output workspace. The current loader is optimized to work with NXcanSAS data which was saved using the :ref:`algm-SaveNXcanSAS` algorithm.

If the file contains transmission workspaces then they can be loaded into separate workspaces.


Usage
-----

**Example**

.. testcode:: LoadNXcanSAS

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
