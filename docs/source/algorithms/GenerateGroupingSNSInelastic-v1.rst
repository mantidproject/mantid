.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Generate grouping files for ARCS, CNCS, HYSPEC, and SEQUOIA, by grouping
py pixels along a tube and px tubes. py is 1, 2, 4, 8, 16, 32, 64, or
128. px is 1, 2, 4, or 8.

To select older instrument definition file, set instrument to "InstrumentDefinitionFile" and an input option will appear, allowing users to navigate to the correct directory.

.. Note ::

    All parameters are strings. Using integers for AlongTubes or AcrossTubes will cause errors.
    Only instrument definition files from ARCS, SEQUOIA, CNCS, HYSPEC are supported.
    If both an instrument and InstrumentDefinitionFile are selected, InstrumentDefinitionFile will be ignored.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: GenerateGroupingSNSInelastic

    # create some grouping file
    import mantid
    outputFilename=mantid.config.getString("defaultsave.directory")+"cncs.xml"
    GenerateGroupingSNSInelastic(outputFilename,AlongTubes="16",AcrossTubes="4",Instrument="CNCS")

    #check that it works
    import os.path
    if(os.path.isfile(outputFilename)):
        print("Found file cncs.xml")

    ws=Load("CNCS_7860")
    wsg=GroupDetectors(ws,outputFilename)
    print("The grouped workspace has {} histograms".format(wsg.getNumberHistograms()))

.. testcleanup:: GenerateGroupingSNSInelastic

   DeleteWorkspace(ws)
   DeleteWorkspace(wsg)
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"cncs.xml"
   os.remove(filename)

Output:

.. testoutput:: GenerateGroupingSNSInelastic

    Found file cncs.xml
    The grouped workspace has 800 histograms

If one would use LoadDetectorsGroupingFile on cncs.xml one would get a workspace that looks like

.. figure:: /images/GenerateGroupingSNSInelastic.png
   :alt: GenerateGroupingSNSInelastic.png

.. categories::

.. sourcelink::
