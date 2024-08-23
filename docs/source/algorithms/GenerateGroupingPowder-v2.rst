.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

For powder samples, with no texture, the scattering consists only of
rings. This algorithm reads a workspace and an angle step, then
generates a grouping file (.xml or .nxs) and an optional par file (.par),
by grouping detectors in one of several methods:

* Divide ring into circular sectors labeled i\*step to (i+1)\*step (see note below).
  The original and default behavior.
* Divide sphere into spherical sectors, similarly to above but including azimuthal angles (see note below).
  Use NumberByAngle set to true and specify AzimuthalStep and AzimuthalStart.
* Divide ring into circular sectors labeled in order, from large 2theta to low 2theta.
  Use NumberByAngle set to false and specify only AngleStep (or set AzimuthalStep to its default at 360).
* Divide ring into circular sectors labeled in order, splitting left/right sides from large 2theta to low 2theta.
  One side is labeled first, then the other.
  Use NumberByAngle set to false, specify AngleStep, and specify AzimuthalStep as anything other than 360.

The par file is required
for saving in the NXSPE format, since Mantid does not correctly
calculate the angles for detector groups. It will contain
average distances to the detector groups, and average scattering angles.
The x and y extents in the par file are radians(step)\*distance and
0.01, and are not supposed to be accurate.  A par file can only be saved using the original default labeling scheme.

The grouping file (.xml) can be used with :ref:`GroupDetectors
<algm-GroupDetectors>` to perform the grouping.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: GenerateGroupingPowder_default

    # create some grouping file
    import mantid
    outputFilename=mantid.config.getString("defaultsave.directory")+"powder.xml"

    #load some file
    ws=Load("CNCS_7860")

    #generate the files
    GenerateGroupingPowder(ws,10, GroupingFilename=outputFilename)

    #check that it works
    import os.path
    if(os.path.isfile(outputFilename)):
        print("Found file powder.xml")
    if(os.path.isfile(mantid.config.getString("defaultsave.directory")+"powder.par")):
        print("Found file powder.par")
    wsg=GroupDetectors(ws,outputFilename)
    print("The grouping workspace has {} histograms".format(wsg.getNumberHistograms()))

.. testcleanup:: GenerateGroupingPowder_default

   DeleteWorkspace(ws)
   DeleteWorkspace(wsg)
   import os,mantid
   filename=mantid.config.getString("defaultsave.directory")+"powder.xml"
   os.remove(filename)
   filename=mantid.config.getString("defaultsave.directory")+"powder.par"
   os.remove(filename)

Output:

.. testoutput:: GenerateGroupingPowder_default

    Found file powder.xml
    Found file powder.par
    The grouping workspace has 14 histograms

----

Similarly, one could instead specify the grouping workspace:

.. testcode:: GenerateGroupingPowder_GroupingWorkspace

    #load some file
    ws=Load("CNCS_7860")
    gws_name= "a_nice_name_for_grouping_workspace"

    #generate the files
    GenerateGroupingPowder(ws,10, GroupingWorkspace=gws_name)

    #check that it works
    gws = mtd[gws_name]
    print("The grouping workspace has {} histograms".format(gws.getNumberHistograms()))

.. testcleanup:: GenerateGroupingPowder_GroupingWorkspace

   DeleteWorkspace(ws)
   DeleteWorkspace(gws)



Output:

.. testoutput:: GenerateGroupingPowder_GroupingWorkspace

    The grouping workspace has 51200 histograms



If one would use LoadDetectorsGroupingFile on powder.xml one would get a workspace that looks like

.. figure:: /images/GenerateGroupingPowder.png
   :alt: GenerateGroupingPowder.png

.. categories::

.. sourcelink::
