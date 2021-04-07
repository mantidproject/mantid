
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates a normalization MD workspace for single crystal diffraction experiments.
Trajectories of each detector in reciprocal space are calculated, and the flux is integrated between intersections with each
MDBox. A brief introduction to the multi-dimensional data normalization can be found :ref:`here <MDNorm>`.

The algorithm :ref:`MDNormSCDPreprocessIncoherent
<algm-MDNormSCDPreprocessIncoherent>` can be used to process Vanadium
data for the Solid Angle and Flux workspaces.

.. Note::
    As of :ref:`Release 4.0.0 <v4.0.0>`, the algorithm can handle merged MD workspaces. Make sure all original MDEvent workspaces have the same dimensions

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - MDNormSCD**

.. testcode:: MDNormSCDExample

  try:
    # Setting up the workspaces containing information about the flux and the solid angle (from a vanadium run)
    rawVan=Load(Filename=r'TOPAZ_7892_event.nxs')
    rawVan=ConvertUnits(InputWorkspace=rawVan,Target='Momentum')
    MaskBTP(Workspace=rawVan,Bank="10-16,19-21,23-25,29-35,40-46,49-57,59")
    MaskBTP(Workspace=rawVan,Pixel="0-9,246-255")
    MaskBTP(Workspace=rawVan,Tube="0-9,246-255")
    rawVan=CropWorkspace(InputWorkspace=rawVan,XMin='1.85',XMax='10')

    #Solid angle
    sa=Rebin(InputWorkspace=rawVan,Params='1.85,10,10',PreserveEvents='0')
    SaveNexus(InputWorkspace=sa, Filename="/home/3y9/Desktop/TOPAZ/solidAngle.nxs")

    #flux
    flux=GroupDetectors(InputWorkspace=rawVan,MapFile=r'/home/3y9/Desktop/TOPAZ/grouping.xml')
    DeleteWorkspace(rawVan)
    flux=CompressEvents(InputWorkspace=flux,Tolerance=1e-4)
    flux=Rebin(InputWorkspace=flux,Params='1.85,10,10')
    for i in range(flux.getNumberHistograms()):
        el=flux.getSpectrum(i)
        el.divide(flux.readY(i)[0],0)
    flux=Rebin(InputWorkspace=flux,Params='1.85,10,10')
    flux=IntegrateFlux(flux)
    SaveNexus(InputWorkspace=flux, Filename="/home/3y9/Desktop/TOPAZ/spectra.nxs")

    #data
    #If you have multiple workspaces, add separately the output workspaces, and separately the
    #output normalization workspaces, then divide the two sums
    runs = range(7985,7995,1)
    mdout = None
    mdnorm = None
    for run in runs:
        try:
            MDdata = LoadMD(Filename="/home/3y9/Desktop/TOPAZ/MDdata_"+str(run)+".nxs")
        except:
            data=Load(Filename='TOPAZ_'+str(run)+'_event.nxs')
            data=ConvertUnits(InputWorkspace=data,Target='Momentum')
            MaskBTP(Workspace=data,Bank="10-16,19-21,23-25,29-35,40-46,49-57,59")
            MaskBTP(Workspace=data,Pixel="0-9,246-255")
            MaskBTP(Workspace=data,Tube="0-9,246-255")
            data=CropWorkspace(InputWorkspace=data,XMin='1.85',XMax='10')
            data=Rebin(InputWorkspace=data,Params='1.85,10,10')
            LoadIsawUB(InputWorkspace=data,Filename=r'7995.mat')
            MDdata=ConvertToMD(InputWorkspace=data,QDimensions="Q3D",dEAnalysisMode="Elastic",
                Q3DFrames="HKL",QConversionScales="HKL",
                MinValues="-10,-10,-10",Maxvalues="10,10,10")
            SaveMD(InputWorkspace=MDdata, Filename="/home/3y9/Desktop/TOPAZ/MDdata_"+str(run)+".nxs")

        #running the algorithm
        mdout, mdnorm = MDNormSCD(InputWorkspace='MDdata',
            AlignedDim0='[H,0,0],-8,8,100',
            AlignedDim1='[0,K,0],-8,8,100',
            AlignedDim2='[0,0,L],-8,8,100',
            FluxWorkspace=flux,
            SolidAngleWorkspace=sa,
            TemporaryDataWorkspace=mdout,
            TemporaryNormalizationWorkspace=mdnorm)

    normalized=DivideMD('mdout','mdnorm')
  except:
    pass

.. testoutput:: MDNormSCDExample




References
----------

The source for how this calculation is done is

#. T.M. Michels-Clark, A.T. Savici, V.E. Lynch, X.P. Wang and C.M. Hoffmann *Expanding Lorentz and spectrum corrections to large volumes of reciprocal space for single-crystal time-of-flight neutron diffraction.* J Appl Crystallogr **49.2** (2016) doi: `10.1107/S1600576716001369 <http://dx.doi.org/10.1107/S1600576716001369>`_


.. categories::

.. sourcelink::

