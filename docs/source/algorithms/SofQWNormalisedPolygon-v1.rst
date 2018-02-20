.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Converts a 2D workspace from :ref:`units <Unit Factory>` 
of spectrum number/**energy transfer** 
to the intensity as a function of **momentum transfer** :math:`Q` 
and **energy transfer** :math:`\Delta E`. 

.. figure:: /images/RebinnedOutput.png
   :align: center

As shown in the figure, the input grid (pink-shaded parallelopiped,
aligned in scattering angle and energy transfer) is not parallel to the
output grid (square grid, aligned in :math:`Q` and energy). This means
that the output bins will only ever partially overlap the input data. To
account for this, the signal :math:`Y` and errors :math:`E` in the output
bin is calculated as the sum of all the input bins which overlap the
output bin, weighted by the fractional overlap area :math:`F_i`:

.. math:: Y^{\mathrm{out}} = (\sum_i Y^{\mathrm{in}}_i F_i) / \sum_i F_i
.. math:: E^{\mathrm{out}} = \sqrt{\sum_i (E^{\mathrm{in}}_i F_i)^2} / \sum_i F_i

.. warning:: Note that because the output bins contain fractions of multiple
   input bins, the errors calculated for each output bins are no longer
   independent, and so cannot be combined in quadrature. This means that
   rebinning, summing, or integrating the output of this algorithm will 
   give *incorrect error values* because those Mantid algorithms use the
   quadrature formular and assume independent bins. The *signal*, on the
   other hand should still be correct on rebinning. Unary operations, such
   as scaling the signal will not encounter this problem.
   
The algorithm operates in non-PSD mode by default. This means that all
azimuthal angles and widths are forced to zero. PSD mode will determine
the azimuthal angles and widths from the instrument geometry. This mode
is activated by placing the following named parameter in the instrument definition 
file: *detector-neighbour-offset*. The integer value of this parameter
should be the number of pixels that separates two pixels at the same
vertical position in adjacent tubes. Note that in both non-PSD and PSD
modes, the scattering angle widths are determined from the detector
geometry and may vary from detector to detector as defined by the
instrument definition files.

See :ref:`algm-SofQWCentre` for centre-point binning or :ref:`algm-SofQWPolygon`
for simpler and less precise but faster binning strategies. The speed-up
is from ignoring the azimuthal positions of the detectors (as for the non-PSD
mode in this algorithm) but in addition, :ref:`algm-SofQWPolygon` treats 
all detectors as being the same, and characterised by a single width in
scattering angle. Thereafter, it weights the signal and error by the fractional
overlap, but does not then scale the weighted sum by :math:`\sum_i F_i`.

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
