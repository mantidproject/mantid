.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm transforms an input workspace in wavelength to :math:`Q_{x}, Q_{z}`
or momentum space for reflectometry workspaces. Prior to the transformation, the
algorithm corrects the detector position to an angle :math:`\theta_f`, where
:math:`\theta_f` is extracted from the log value :literal:`stheta`, if it exists;
otherwise, no correction is done, and detectors are assumed to be at :math:`\theta_f`.

For conversion of single histogram workspaces to :math:`Q_{z}`, see
:ref:`ConvertUnits <algm-ConvertUnits>` or
:ref:`ReflectometryMomentumTransfer <algm-ReflectometryMomentumTransfer>`.

This version of the algorithm takes :math:`\theta_f` from the detector :math:`2\theta`,
whereas version 2 takes :math:`\theta_f` from :math:`2\theta - \theta_i`.

Prerequisites
#############

The workspace spectrum axis should be converted to signed\_theta using
:ref:`algm-ConvertSpectrumAxis` and the x axis should be
converted to Wavelength using :ref:`algm-ConvertUnits` before
running this algorithm. Histogram input workspaces are expected.

The algorithm will look for a specific log value called :literal:`stheta`, which
contains the incident theta angle :math:`\theta_i`. If the input
workspace does not contain this value, or if you wish to override this
value you can do so by providing your own :literal:`IncidentTheta` property and
enabling :literal:`OverrideIncidentTheta`.

The algorithm also has the ability to produce additional debugging information from the Tableworkspace
that can be used to create a patch plot of results before any 2D fractional rebinning has happened.

To create this table there are certain properties that must be present in the algorithm:

- The transform method must be NormalisedPolygon
- The option to Output as MD workspace must not be selected
- The option to DumpVertexes must be selected

Transformations
###############

Output workspaces are always 2D MD Histogram workspaces, but the
algorithm will perform one of three possible transformations.

-  Convert to :math:`Q_x`, :math:`Q_z`
-  Convert to :math:`K_i`, :math:`K_f`
-  Convert to :math:`P_i-P_f`, :math:`P_i+P_f`. Note that P and K are
   interchangeable.

where

:math:`Q_x = \frac{2\pi}{\lambda}(cos\theta_f - cos\theta_i)`

:math:`Q_z = \frac{2\pi}{\lambda}(sin\theta_f + sin\theta_i)`

:math:`K_i = \frac{2\pi}{\lambda}sin\theta_i`

:math:`K_f = \frac{2\pi}{\lambda}sin\theta_f`


After Transformation
####################

You will usually want to rebin using :ref:`algm-BinMD` or
:ref:`algm-SliceMD` after transformation because the output workspaces
are not regularly binned.

Binning Methods
###############

The *Method* property allows the binning method used when applying the
coordinate transformations to be selected. The default method,
*Centre*, takes center point of each input bin, and locates the
corresponding output bin, adding the input bins value to it. Centre point rebinning is faster.

*NormalisedPolygon* is a more sophisticated approach. It constructs
a polygon using the boundaries of the input bin, then transforms that polygon
into the output coordinates, and then searches for intersections with the
output bins. The value added to each output bin is proportional to size of the
overlap with the input bin. The normalised polygon approach gives better accuracy.

Usage
-----
Normalised Polygon Transformation
#################################

**Example - Normalised Polygon transformation**

.. testcode:: ExConvReflQSimple

    workspace_name = "POLREF4699"
    workspace_nexus_file = workspace_name + ".nxs"

    Load(Filename=workspace_nexus_file,OutputWorkspace=workspace_name)
    X = mtd[workspace_name]
    X = ConvertUnits(InputWorkspace=X,Target="Wavelength",AlignBins="1")
    # Reference intensity to normalise by
    CropWorkspace(InputWorkspace=X,OutputWorkspace='Io',XMin=0.8,XMax=14.5,StartWorkspaceIndex=2,EndWorkspaceIndex=2)
    # Crop out transmission and noisy data
    CropWorkspace(InputWorkspace=X,OutputWorkspace='D',XMin=0.8,XMax=14.5,StartWorkspaceIndex=3)
    Io=mtd['Io']
    D=mtd['D']

    # Perform the normalisation step
    Divide(LHSWorkspace=D,RHSWorkspace=Io,OutputWorkspace='I',AllowDifferentNumberSpectra='1',ClearRHSWorkspace='1')
    I=mtd['I'][0]

    # Move the detector so that the detector channel matching the reflected beam is at 0,0
    PIX = 1.1E-3 #m
    SC = 75
    avgDB = 29
    zOffset = -PIX * ((SC - avgDB) * 0.5 + avgDB)
    MoveInstrumentComponent(Workspace = I, ComponentName = "lineardetector", X = 0, Y = 0, Z = zOffset)

    # Should now have signed theta vs Lambda
    ConvertSpectrumAxis(InputWorkspace=I,OutputWorkspace='SignedTheta_vs_Wavelength',Target='signed_theta')

    qxqy, vertexes_qxqy = ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength', OutputDimensions='Q (lab frame)', Extents='-0.0005,0.0005,0,0.12', OutputAsMDWorkspace=False,Method='NormalisedPolygon')

    kikf, vertexes_kikf = ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength', OutputDimensions='K (incident, final)', Extents='0,0.05,0,0.05', OutputAsMDWorkspace=False,Method='NormalisedPolygon')

    pipf, vertexes_pipf = ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength', OutputDimensions='P (lab frame)', Extents='0,0.1,-0.02,0.15', OutputAsMDWorkspace=False,Method='NormalisedPolygon')

    print("{} {}".format(qxqy.getDimension(0).name, qxqy.getDimension(1).name))
    print("{} {}".format(kikf.getDimension(0).name, kikf.getDimension(1).name))
    print("{} {}".format(pipf.getDimension(0).name, pipf.getDimension(1).name))


Output:

.. testoutput:: ExConvReflQSimple

    Qx Qz
    Ki Kf
    Pz_i + Pz_f Pz_i - Pz_f

Plot of the SignedTheta vs Wavelength workspace:
================================================
Before the ConvertToReflectometryQ algorithm is executed in the usage example above, the plot of the 'SignedTheta_vs_Wavelength' workspace should resemble this plot:

.. figure:: /images/SignedThetaVSlam_plot.png
   :alt: plot of Signed theta vs lambda.

Patch Plot with Dumped Vertexes
###############################

ConvertToReflectometryQ has the functionality to produce a table of vertexes before they are
fitted to a normalised polygon. The plotting of these vertexes results in a patch plot that can be
achieved by running the algorithm below.

**Example - Patch Plot using the Dumped vertexes from QxQy Transformation**

.. code-block:: python

    import numpy as np
    import matplotlib
    from matplotlib.patches import Polygon
    from matplotlib.collections import PatchCollection
    import matplotlib.pyplot as plt
    from matplotlib.colors import LogNorm

    # full reduction on workspace
    Load(Filename='data_th_lam.nxs', OutputWorkspace='data_th_lam')
    CropWorkspace('data_th_lam', StartWorkspaceIndex=124, OutputWorkspace='data_th_lam')
    data_th_lam = Rebin('data_th_lam', [1e-2])

    out_ws, dump_vertexes = ConvertToReflectometryQ(InputWorkspace='data_th_lam',OutputWorkspace='QxQy_poly', OutputDimensions='Q (lab frame)',
    Extents='-0.0005,0.0005,-0,0.2', OutputAsMDWorkspace=False,Method='NormalisedPolygon',  IncidentTheta=0.44, OverrideIncidentTheta=True, NumberBinsQx=100, NumberBinsQz=100,DumpVertexes=True, OutputVertexes='dump_vertexes')

    #plot the conversion
    plotSlice(out_ws)

    def patch_plot(vertex_table):
        fig, ax = plt.subplots()

        patches = list()
        colors = list()
        polygon_vertexes = list()

        for vertex in vertex_table:
            #Column of vertex i.e 'Qx' in this case, is dependent on the type of transform.
            #'Ki' and 'Kf' are used for the K transformation.
            #'Pi+Pf' and 'Pi-Pf' are used for the P transformation.
            polygon_vertexes.append((vertex['Qx'], vertex['Qy'] ))
            if len(polygon_vertexes) == 4:
                poly = Polygon(polygon_vertexes, True,edgecolor='none',linewidth=0)
                patches.append(poly)
                colors.append(vertex['CellSignal'])
                polygon_vertexes = list()

        p = PatchCollection(patches, cmap=matplotlib.cm.jet,norm=LogNorm(vmin=1e-3, vmax=1e5),linewidths=(0,))
        p.set_array(np.array(colors))
        ax.add_collection(p)
        plt.colorbar(p)
        axes = plt.gca()
        axes.set_xlim([-0.0004,0.0004])
        axes.set_ylim([0,0.2])

        fig.show()

    threadsafe_call(patch_plot, dump_vertexes)

**Output:**

Patch plot for QxQy Transformation:

.. figure:: /images/ConvertToReflectometryQ_PatchPlotQ.png
   :alt: patch plot of dumped vertexes using Q transformation

Patch plots from other transformations
######################################

Patch plots can also be produced using the other Transformations :math:`K_i, K_f` and :math:`P_i-P_f, P_i+P_f`


**Patch plot for KiKf Transformation:**


.. figure:: /images/ConvertToReflectometryQ_PatchPlotK.PNG
   :alt: patch plot of dumped vertexes using K transformation


**Patch plot for P Transformation:**


.. figure:: /images/ConvertToReflectometryQ_PatchPlotP.PNG
   :alt: patch plot of dumped vertexes using P transformation

.. categories::

.. sourcelink::
