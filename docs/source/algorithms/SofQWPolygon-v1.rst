.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Converts a 2D workspace in :ref:`units <Unit Factory>` 
of spectrum number/**energy transfer** to 
the intensity as a function of momentum transfer 
:math:`Q` and energy transfer :math:`\Delta E`. 

The rebinning is done as a weighted sum of overlapping polygons. The polygon 
in :math:`Q-\Delta E` space is calculated from the energy bin boundaries and 
the detector scattering angle :math:`2\theta`. The detectors (pixels) are 
assumed to be uniform, and characterised by a single angular width 
:math:`\Delta2\theta`. This is calculated from the nominal :math:`2\theta` of 
each detector; this algorithm does not utilize the `DetectorTwoThetaRanges` 
optional input property. The signal and error of the rebinned data (in 
:math:`Q-\Delta E` space) is then the sum of the contributing pixels in each 
bin weighted by their fractional overlap area. Unlike the more precise 
:ref:`algm-SofQWNormalisedPolygon` algorithm, these fractional weights are not 
thereafter retained in the workspace produced by this algorithm.

See :ref:`algm-SofQWCentre` for centre-point binning.
Alternatively, see :ref:`algm-SofQWNormalisedPolygon` for a
more complex and precise (but slower) binning strategy, where the actual
detector shape is calculated to obtain the input polygon.

Usage
-----

**Example - simple transformation of inelastic workspace:**

.. testcode:: SofQWPolygon

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra values
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into MD workspace 
   ws=SofQWPolygon(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
  
   print("The converted X-Y values are:")
   Xrow=ws.readX(59);
   Yrow=ws.readY(59);   
   line1= " ".join('! {0:>6.2f} {1:>6.2f} '.format(Xrow[i],Yrow[i]) for i in range(0,10))
   print(line1 + " !")
   line2= " ".join('! {0:>6.2f} {1:>6.2f} '.format(Xrow[i],Yrow[i]) for i in range(10,20))
   print(line2 + " !")
   print('! {0:>6.2f} ------- !'.format(Xrow[20]))



.. testcleanup:: SofQWPolygon

   DeleteWorkspace(ws)
   
**Output:**

.. testoutput:: SofQWPolygon

   The converted X-Y values are:
   ! -10.00  12.79  !  -9.00  17.63  !  -8.00  17.86  !  -7.00  18.12  !  -6.00  18.46  !  -5.00  18.69  !  -4.00  19.24  !  -3.00  19.67  !  -2.00  18.49  !  -1.00  12.00  !
   !   0.00  17.08  !   1.00  22.32  !   2.00  23.26  !   3.00  24.46  !   4.00  25.96  !   5.00  21.96  !   6.00  25.10  !   7.00  33.65  !   8.00  35.54  !   9.00  43.86  !
   !  10.00 ------- !


.. categories::

.. sourcelink::
