.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Converts a 2D workspace from :py:obj:`Units <mantid.kernel.UnitFactoryImpl>`
of spectrum number/**energy transfer**
to the intensity as a function of **momentum transfer** :math:`Q`
and **energy transfer** :math:`\Delta E`.

The details of the Normalised Polygon technique for rebinning are given in
:ref:`FractionalRebinning <FractionalRebinning>`.

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

Usage
-----

**Example - simple transformation of inelastic workspace:**

.. testcode:: SofQWNormalisedPolygon

   # create sample inelastic workspace for MARI instrument containing 1 at all spectra
   ws=CreateSimulationWorkspace(Instrument='MAR',BinParams='-10,1,10')
   # convert workspace into Matrix workspace with Q-dE coordinates
   ws=SofQWNormalisedPolygon(InputWorkspace=ws,QAxisBinning='-3,0.1,3',Emode='Direct',EFixed=12)

   print("The converted X-Y values are:")
   Xrow=ws.x(59);
   Yrow=ws.y(59);
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
