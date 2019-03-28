.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Converts a 2D workspace from :ref:`units <Unit Factory>` 
of spectrum number/**energy transfer** 
to the intensity as a function of **momentum transfer** :math:`Q` 
and **energy transfer** :math:`\Delta E`. 

.. figure:: /images/RebinnedOutputStep1.png
   :align: center

As shown in the figure, the input grid (pink-shaded parallelopiped,
aligned in scattering angle and energy transfer) is not parallel to the
output grid (square grid, aligned in :math:`Q` and energy). This means
that the output bins will only ever partially overlap the input data. To
account for this, the signal :math:`Y` and errors :math:`E` in the new
bins are calculated as:

.. math:: Y^{\mathrm{new}} = \sum_i Y^{\mathrm{old}}_i F_i
.. math:: E^{\mathrm{new}} = \sqrt{\sum_i (E^{\mathrm{old}}_i)^2 F_i}

In addition to weighting the signal and error values, the total
fractional weights:

.. math:: F^{\mathrm{new}} = \sum_i F_i

are also stored in the output of this algorithm which is a new workspace
type: **RebinnedOutput**. To see why this is needed, consider rebinning
the output on a larger grid:

.. figure:: /images/RebinnedOutputStep2.png
   :align: center

If the fractional weights are not chained as shown, then the area
shaded in a lighter blue under :math:`A_1` (where originally there was
no data) would be included in the weights, which would be an
overestimate of the actual weights, leading to an overestimate of the
signal and error.

Finally, if there is a bin in the output grid which only has a very
small overlap with the input grid (for example at the edges of the
detector coverage), the fractional weight :math:`F` of this bin, and
hence its signal :math:`Y` and error :math:`E` would be very small
compared to its neighbours. Thus, for display purposes, the actual
signal and errors stored in a RebinnedOutput are renomalised by the
weights:

.. math:: Y^{\mathrm{display}} = Y^{\mathrm{new}} / F^{\mathrm{new}}
.. math:: E^{\mathrm{display}} = E^{\mathrm{new}} / F^{\mathrm{new}}

at the end of the algorithm. The biggest consequence of this method is
that in places where there are no counts (:math:`Y=0`) and no acceptance
(no fractional areas, :math:`F=0`), :math:`Y/F=`\ **nan**\ -s will
result.

The algorithm operates in *non-PSD mode* by default. This means that the 
scattering angle :math:`2\theta` range covered by a detector is calculated for 
each detector individually. For grouped detectors, it is the minimum and 
maximum :math:`2\theta` of all detectors in the group. The computation is 
accurate for simple detector shapes (cylinder, cuboid); for other shapes a 
more rough method is used. It is possible to provide precalculated 
per-detector :math:`2\theta` values using the ``DetectorTwoThetaRanges`` input 
property.

*PSD mode* will determine the detector :math:`2\theta` ranges from the 
instrument geometry. This mode is activated by placing the following named 
parameter in the instrument definition file: *detector-neighbour-offset*. The 
integer value of this parameter should be the number of pixels that separates 
two pixels at the same vertical position in adjacent tubes.

See :ref:`algm-SofQWCentre` for centre-point binning or :ref:`algm-SofQWPolygon`
for simpler and less precise but faster binning strategies. The speed-up
is from ignoring the azimuthal positions of the detectors (as for the non-PSD
mode in this algorithm) but in addition, :ref:`algm-SofQWPolygon` treats 
all detectors as being the same, and characterised by a single width in
scattering angle. Thereafter, it weights the signal and error by the fractional
overlap, similarly to that shown in the first figure above, but then discards
the summed weights, producing a **Workspace2D** rather than a
**RebinnedOutput** workspace.

Usage
-----

**Example - simple transformation of inelastic workspace:**

.. testcode:: SofQWNormalisedPolygon

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra 
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into Matrix workspace with Q-dE coordinates 
   ws=SofQWNormalisedPolygon(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)
  
   print("The converted X-Y values are:")
   Xrow=ws.readX(59);
   Yrow=ws.readY(59);   
   line1= " ".join('! {0:>6.2f} {1:>6.2f} '.format(Xrow[i],Yrow[i]) for i in range(0,10))
   print(line1 + " !")
   line2= " ".join('! {0:>6.2f} {1:>6.2f} '.format(Xrow[i],Yrow[i]) for i in range(10,20))
   print(line2 + " !")
   print('! {0:>6.2f} ------- !'.format(Xrow[20]))



.. testcleanup:: SofQWNormalisedPolygon

   DeleteWorkspace(ws)
   
**Output:**

.. testoutput:: SofQWNormalisedPolygon

   The converted X-Y values are:
   ! -10.00   1.00  !  -9.00   1.00  !  -8.00   1.00  !  -7.00   1.00  !  -6.00   1.00  !  -5.00   1.00  !  -4.00   1.00  !  -3.00   1.00  !  -2.00   1.00  !  -1.00   1.00  !
   !   0.00   1.00  !   1.00   1.00  !   2.00   1.00  !   3.00   1.00  !   4.00   1.00  !   5.00   1.00  !   6.00   1.00  !   7.00   1.00  !   8.00   1.00  !   9.00   1.00  !
   !  10.00 ------- !


.. categories::

.. sourcelink::
