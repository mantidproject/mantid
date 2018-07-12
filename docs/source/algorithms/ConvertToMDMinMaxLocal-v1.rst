.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate min-max input values for selected workspace and MD transformation, 
choosen from `MD Transformation factory <http://www.mantidproject.org/MD_Transformation_factory>`_.

Used as helper algorithm for :ref:`algm-ConvertToMD` but can aslo be deployed separately 
to evaluate the MD transformation limits for the current workspace.

Initiates the same as :ref:`algm-ConvertToMD` algorithm transformation from the
`MD Transformation factory <http://www.mantidproject.org/MD_Transformation_factory>`_ and uses this 
transformation to evaluate all points where the transformation can achieve extrema 
for each workspace spectra. Then goes through all extrema points, calculates min/max 
values for each spectra and select global min-max transformation values for 
this workspace.

For example, given input workspace in the units of energy transfer and
requesting :math:`|Q|` inelastic transformation, the algorithm looks through
all spectra of the input workspace and identifies minimal, maximal and
an extremal [#f1]_ energy transfer for the input spectra. Then it runs 
:math:`|Q|,dE` conversion for these energy transfer points and loops through all
spectra of the workspace to identify :math:`|Q|_{min}, |Q|_{max}` and 
:math:`dE_{min},dE_{max}` values.

.. rubric:: Note

.. [#f1] extremal energy transfer for **|Q|** transformation occurs at some
   energy transfer where momentum transfer is maximal. Its value depend on
   polar angle of the detector.
   
Usage
-----

**Example - Find min-max values for |Q| transformation :**

.. testcode:: ExConvertToMDMinMaxLocalQ

    # Simulates Load of a workspace with all necessary parameters #################
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[-50,2,50],UnitX='DeltaE')
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
    # evaluate |Q| transformation limits
    minn,maxx = ConvertToMDMinMaxLocal(InputWorkspace=detWS,QDimensions='|Q|',dEAnalysisMode='Direct')
    # Look at sample results:    
    print('MD workspace limits:')
    print('|Q|_min: {0:10f}, dE_min: {1:10f}'.format(minn[0],minn[1]))
    print('|Q|_max: {0:10f}, dE_max: {1:10f}'.format(maxx[0],maxx[1]))
    
.. testcleanup:: ExConvertToMDMinMaxLocalQ

   DeleteWorkspace(detWS)
   DeleteWorkspace('PreprocessedDetectorsWS')   

**Output:**

.. testoutput:: ExConvertToMDMinMaxLocalQ

    MD workspace limits:
    |Q|_min:   0.299713, dE_min: -50.000000
    |Q|_max:  11.102851, dE_max:  50.000000

**Example -- Find min-max values for Q3D transformation, while converting TOF to energy transfer :**    

.. testcode:: ExConvertToMDMinMaxLocalQ3D
    
    # Simulates Load of a workspace with all necessary parameters #################    
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[20000,20,400000],UnitX='TOF')
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
    
    # evaluate Q3D transformation limits, which includes converting units    
    minn,maxx = ConvertToMDMinMaxLocal(InputWorkspace=detWS,QDimensions='Q3D',dEAnalysisMode='Direct')
    print('Min values::  Qx: {0:10f}, Qy: {1:10f}, Qz: {2:10f},  dE:{3:10f}'.format(minn[0],minn[1],minn[2],minn[3]))
    print('Max values::  Qx: {0:10f}, Qy: {1:10f}, Qz: {2:10f},  dE:{3:10f}'.format(maxx[0],maxx[1],maxx[2],maxx[3]))    
       
.. testcleanup:: ExConvertToMDMinMaxLocalQ3D

   DeleteWorkspace(detWS)
   DeleteWorkspace('PreprocessedDetectorsWS')   

**Output:**

.. testoutput:: ExConvertToMDMinMaxLocalQ3D

   Min values::  Qx:  -0.067199, Qy:  -0.090211, Qz:   4.617771,  dE: 51.680897
   Max values::  Qx:   0.067199, Qy:   0.392381, Qz:   5.282783,  dE: 51.999462
  
**Example -- Finding min-max values for CopyToMD transformation uses the source workspace limits :**    
  
.. testcode:: ExConvertToMDMinMaxLocalCopyToMD
  
   # Simulates Load of a workspace with all necessary parameters #################  
   detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[200,2,20000],UnitX='TOF')
   AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
   minn,maxx = ConvertToMDMinMaxLocal(InputWorkspace=detWS,QDimensions='CopyToMD',dEAnalysisMode='Direct',OtherDimensions='Ei')
   # Look at sample results:    
   print('MD workspace limits:')
   print('TOF_min: {0:10f}, Ei_min: {1:10f}'.format(minn[0],minn[1]))
   print('TOF_max: {0:10f}, Ei_max: {1:10f}'.format(maxx[0],maxx[1]))

.. testcleanup:: ExConvertToMDMinMaxLocalCopyToMD

   DeleteWorkspace(detWS)
   DeleteWorkspace('PreprocessedDetectorsWS')   

**Output:**

.. testoutput:: ExConvertToMDMinMaxLocalCopyToMD

    MD workspace limits:
    TOF_min: 200.000000, Ei_min:  52.000000
    TOF_max: 20000.000000, Ei_max:  52.000000

   
  
.. categories::

.. sourcelink::
