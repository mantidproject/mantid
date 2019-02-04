.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

For powder samples, with no texture, the scattering consists only of
rings. This algorithm reads a workspace and an angle step, then
generates a grouping file (.xml) and an optional par file (.par), by grouping
detectors in intervals i\*step to (i+1)\*step. The par file is required
for saving in the NXSPE format, since Mantid does not correctly
calculate the angles for detector groups. It will contain
average distances to the detector groups, and average scattering angles.
The x and y extents in the par file are radians(step)\*distance and
0.01, and are not supposed to be accurate.

The grouping file (.xml) can be used with :ref:`GroupDetectors 
<algm-GroupDetectors>` to perform the grouping.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: GenerateGroupingPowder

    # create some grouping file
    import mantid
    outputFilename=mantid.config.getString("defaultsave.directory")+"powder.xml"
        
    #load some file
    ws=Load("CNCS_7860")
    
    #generate the files
    GenerateGroupingPowder(ws,10,outputFilename)
    
    #check that it works
    import os.path
    if(os.path.isfile(outputFilename)):
        print("Found file powder.xml")
    if(os.path.isfile(mantid.config.getString("defaultsave.directory")+"powder.par")):
        print("Found file powder.par")
    wsg=GroupDetectors(ws,outputFilename)
    print("The grouped workspace has {} histograms".format(wsg.getNumberHistograms()))

.. testcleanup:: GenerateGroupingPowder

   DeleteWorkspace(ws)
   DeleteWorkspace(wsg)
   import os,mantid   
   filename=mantid.config.getString("defaultsave.directory")+"powder.xml"
   os.remove(filename)
   filename=mantid.config.getString("defaultsave.directory")+"powder.par"
   os.remove(filename)
       
Output:

.. testoutput:: GenerateGroupingPowder

    Found file powder.xml
    Found file powder.par
    The grouped workspace has 14 histograms

If one would use LoadDetectorsGroupingFile on powder.xml one would get a workspace that looks like

.. figure:: /images/GenerateGroupingPowder.png
   :alt: GenerateGroupingPowder.png
   
.. categories::

.. sourcelink::
