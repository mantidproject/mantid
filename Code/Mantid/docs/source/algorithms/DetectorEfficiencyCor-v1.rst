.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The probability of neutron detection by each detector in the
:ref:`workspace <workspace>` is calculated from the neutrons' kinetic
energy, angle between their path and the detector axis, detector gas
pressure, radius and wall thickness. The detectors must be cylindrical
and their :sup:`3`\ He partial pressure, wall thickness and radius
are attached to the instrument stored in the input workspace, 
The first parameter is in atmospheres and the last two in metres. 
That workspace then needs to be converted so that its
X-values are in :ref:`units <Unit Factory>` of energy transfer, e.g. using
the :ref:`algm-ConvertUnits` algorithm.

To estimate the true number of neutrons that entered the detector the
counts in each bin are divided by the detector efficiency of that
detector at that energy. The efficiency iteslef is calculated from 
the forumula, tabulated within the algorithm. 

The numbers of counts are then multiplied by the value of
:math:`k_i/k_f` for each bin. In that formula :math:`k_i` is the
wavenumber a neutron leaving the source (the same for all neutrons) and
:math:`k_f` is the wavenumber on hitting the detector (dependent on the
detector and energy bin). They're calculated, in angstrom\ :sup:`-1`, as

| :math:`k_i = \sqrt{\frac{E_i}{2.07212466}}`
| :math:`k_f = \sqrt{\frac{E_i - \Delta E}{2.07212466}}`

where :math:`E_i` and :math:`\Delta E` are energies in meV, the inital
neutron kinetic energy and the energy lost to the sample respectively.

Note: it is not possible to use this :ref:`algorithm <algorithm>` to
correct for the detector efficiency alone. One solution to this is to
divide the output of the algorithm by :math:`k_i/k_f` calculated as above.

Usage
-----

**Example - Correct detectors efficiency:**

.. testcode:: ExDetectorEfficiencyCorr

    # Simulates Load of a workspace with all necessary parameters #################
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[-50,2,50],UnitX='DeltaE')
    detWS.dataE(0)[range(0,50)]=1
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');    
    
    # Correct detectors efficiency 
    corWS = DetectorEfficiencyCor(detWS)
    corWS = CorrectKiKf(corWS,EMode='Direct')

    #
    # Look at sample results:
    print 'part of the corrected workspace:'
    for i in xrange(15,30): 
       print detWS.readX(0)[i],detWS.readY(0)[i],corWS.readY(0)[i],detWS.readE(0)[i],corWS.readE(0)[i]



.. testcleanup:: ExDetectorEfficiencyCorr

   DeleteWorkspace(detWS)
   DeleteWorkspace(corWS)   

**Output:**

.. testoutput:: ExDetectorEfficiencyCorr

   part of the corrected workspace:
   -20.0 1.0 1.11489184233 1.0 1.11489184233
   -18.0 1.0 1.12451654494 1.0 1.12451654494
   -16.0 1.0 1.13460358926 1.0 1.13460358926
   -14.0 1.0 1.14519004998 1.0 1.14519004998
   -12.0 1.0 1.15631723061 1.0 1.15631723061
   -10.0 1.0 1.16803129778 1.0 1.16803129778
   -8.0 1.0 1.180384035 1.0 1.180384035
   -6.0 1.0 1.19343374325 1.0 1.19343374325
   -4.0 1.0 1.20724632323 1.0 1.20724632323
   -2.0 1.0 1.22189658402 1.0 1.22189658402
   0.0 1.0 1.23746983599 1.0 1.23746983599
   2.0 1.0 1.25406384358 1.0 1.25406384358
   4.0 1.0 1.2717912377 1.0 1.2717912377
   6.0 1.0 1.29078252032 1.0 1.29078252032
   8.0 1.0 1.31118984059 1.0 1.31118984059

.. categories::
