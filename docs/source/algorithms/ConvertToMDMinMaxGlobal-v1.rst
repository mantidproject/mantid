.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates the *MinValues* and *MaxValues* limits
produced by the :ref:`algm-ConvertToMD` algorithm for three MD transformation possibilities
namely **CopyToMD**, **|Q|** and **Q3D** [#f1]_. To estimate these limits the algorithm 
uses the following procedure:

-  If **QDimensions** is **CopyToMD** the first value in *MinValues* is going to
   be the workspace minimum X coordinate, and the first value in
   MaxValues is going to be the maximum X coordinate
-  If **QDimensions** is **|Q|** or **Q3D**, first we calculate the maximum
   momentum transfer, :math:`Q_{max}`. If **dEAnalysisMode** is **Elastic**, we convert to
   Momentum units, find the maximum value, and multiply by 2, since the
   maximum momentum transfer occurs when the incident beam and the
   scattered beam are anti-parallel.
-  If **dEAnalysisMode** is **Direct** or **Indirect**, we convert to **DeltaE** units,
   find the minimum and maximum (:math:`dE_{min},dE_{max}`), calculate to :math:`k_{i}` and :math:`k_{f}`.
   The maximum momentum transfer is :math:`k_{i}+k_{f}`.
-  If **QDimensions** is **|Q|**, the first value of the *MinValues* is 0, and
   the first value of *MaxValues* is :math:`Q_{max}`
-  If QDimensions is **Q3D**, and **Q3DFrames** is **Q**, the first three values of
   the *MinValues* are :math:`-Q_{max};-Q_{max};-Q_{max}`, and the first three values of
   *MaxValues* are :math:`Q_{max};Q_{max};Q_{max}`
-  If ***QDimensions** is **Q3D**, and **Q3DFrames** is **HKL** the first three values of
   the *MinValues* are :math:`-Q_{max}\frac{a}{2\pi};-Q_{max}\frac{b}{2\pi};-Q_{max}\frac{c}{2\pi}`
   and the first three values of *MaxValues* are 
   :math:`Q_{max}\frac{a}{2\pi};Q_{max}\frac{b}{2\pi};Q_{max}\frac{c}{2\pi}` [#f2]_ 
-  If **QDimensions** is **|Q|** or **Q3D**, and **dEAnalysisMode** is **Elastic** or
   **Inelastic**, the next value in *MinValues* is **dEmin**, and the next value
   in *MaxValues* is **dEmax**
-  If any **OtherDimensions** are added, the last values in *MinValues*
   (*MaxValues*) are the minimum (maximum) of each of the sample log
   values selected
   
.. rubric:: Notes

.. [#f1] The algorithm does not use `MD Transformation factory <http://www.mantidproject.org/MD_Transformation_factory>`_ so can be used as 
         independent verification of `MD Transformation factory <http://www.mantidproject.org/MD_Transformation_factory>`_ work on a spherical instrument.
.. [#f2] for HKL mode one needs to have an OrientedLattice attached to the sample.
   
Usage
-----

**Example - Find min-max values for |Q| transformation :**

.. testcode:: ExConvertToMDMinMaxGlobalQ

    # Simulates Load of a workspace with all necessary parameters #################
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[-50,2,50],UnitX='DeltaE')
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
    # evaluate |Q| transformation limits
    minn,maxx = ConvertToMDMinMaxGlobal(InputWorkspace=detWS,QDimensions='|Q|',dEAnalysisMode='Direct')
    # Look at sample results:    
    print('MD workspace limits:')
    print('|Q|_min: {0:10f}, dE_min: {1:10f}'.format(minn[0], minn[1]))
    print('|Q|_max: {0:10f}, dE_max: {1:10f}'.format(maxx[0],maxx[1]))
    
.. testcleanup:: ExConvertToMDMinMaxGlobalQ

   DeleteWorkspace(detWS)

**Output:**

.. testoutput:: ExConvertToMDMinMaxGlobalQ

    MD workspace limits:
    |Q|_min:   0.000000, dE_min: -50.000000
    |Q|_max:  12.025534, dE_max:  50.000000
    

**Example -- Find min-max values for Q3D transformation, while converting TOF to energy transfer :**    

.. testcode:: ExConvertToMDMinMaxGlobalQ3D
    
    # Simulates Load of a workspace with all necessary parameters #################    
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[20000,20,400000],UnitX='TOF')
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
    
    # evaluate Q3D transformation limits, which includes converting units    
    minn,maxx = ConvertToMDMinMaxGlobal(InputWorkspace=detWS,QDimensions='Q3D',dEAnalysisMode='Direct')
    print('Min values::  Qx: {0:10f}, Qy: {1:10f}, Qz: {2:10f},  dE:{3:10f}'.format(minn[0],minn[1],minn[2],minn[3]))
    print('Max values::  Qx: {0:10f}, Qy: {1:10f}, Qz: {2:10f},  dE:{3:10f}'.format(maxx[0],maxx[1],maxx[2],maxx[3]))    
       
.. testcleanup:: ExConvertToMDMinMaxGlobalQ3D

   DeleteWorkspace(detWS)

**Output:**

.. testoutput:: ExConvertToMDMinMaxGlobalQ3D

   Min values::  Qx:  -5.401917, Qy:  -5.401917, Qz:  -5.401917,  dE: 51.680898
   Max values::  Qx:   5.401917, Qy:   5.401917, Qz:   5.401917,  dE: 51.999462
     
  
**Example -- Finding min-max values for CopyToMD transformation uses the source workspace limits :**    
  
.. testcode:: ExConvertToMDMinMaxGlobalCopyToMD
  
   # Simulates Load of a workspace with all necessary parameters #################  
   detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[200,2,20000],UnitX='TOF')
   AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
   minn,maxx = ConvertToMDMinMaxGlobal(InputWorkspace=detWS,QDimensions='CopyToMD',dEAnalysisMode='Direct',OtherDimensions='Ei')
   # Look at sample results:    
   print('MD workspace limits:')
   print('TOF_min: {0:10f}, Ei_min: {1:10f}'.format(minn[0],minn[1]))
   print('TOF_max: {0:10f}, Ei_max: {1:10f}'.format(maxx[0],maxx[1]))

.. testcleanup:: ExConvertToMDMinMaxGlobalCopyToMD

   DeleteWorkspace(detWS)

**Output:**

.. testoutput:: ExConvertToMDMinMaxGlobalCopyToMD

    MD workspace limits:
    TOF_min: 200.000000, Ei_min:  52.000000
    TOF_max: 20000.000000, Ei_max:  52.000000

   
.. categories::

.. sourcelink::
