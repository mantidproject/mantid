.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Prerequisites
#############

The workspace spectrum axis should be converted to signed\_theta using
:ref:`algm-ConvertSpectrumAxis` and the x axis should be
converted to Wavelength using :ref:`algm-ConvertUnits` before
running this algorithm. Histogram input workspaces are expected.

The algorithm will looks for a specific log value called *stheta*, which
contains the incident theta angle :math:`theta_i`. If the input
workspace does not contain this value, or if you wish to override this
value you can do so by providing your own *IncidentTheta* property and
enabling *OverrideIncidentTheta*.

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

    # Peform the normalisation step
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
    
    ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='QxQy_poly', OutputDimensions='Q (lab frame)', Extents='-0.0005,0.0005,0,0.12', OutputAsMDWorkspace=False,Method='NormalisedPolygon')
                                
    ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='KiKf_poly', OutputDimensions='K (incident, final)', Extents='0,0.05,0,0.05', OutputAsMDWorkspace=False,Method='NormalisedPolygon')
    
    ConvertToReflectometryQ(InputWorkspace='SignedTheta_vs_Wavelength',OutputWorkspace='PiPf_poly', OutputDimensions='P (lab frame)', Extents='0,0.1,-0.02,0.15', OutputAsMDWorkspace=False,Method='NormalisedPolygon')

    qxqy = mtd['QxQy_poly']
    kikf = mtd['KiKf_poly']
    pipf = mtd['PiPf_poly']
    print qxqy.getDimension(0).getName(), qxqy.getDimension(1).getName()
    print kikf.getDimension(0).getName(), kikf.getDimension(1).getName()
    print pipf.getDimension(0).getName(), pipf.getDimension(1).getName()


Output:

.. testoutput:: ExConvReflQSimple

    Qx Qz
    Ki Kf
    Pz_i + Pz_f Pz_i - Pz_f

.. categories::

.. sourcelink::
