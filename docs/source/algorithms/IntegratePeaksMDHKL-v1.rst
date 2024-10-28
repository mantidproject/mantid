
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

:ref:`IntegratePeaksMDHKL <algm-IntegratePeaksMDHKL>` provides integration of a
:ref:`MDHistoWorkspace <MDHistoWorkspace>` or :ref:`MDEventWorkspace <MDWorkspace>` in 3-dimensions.
The units of the workspace must be HKL.  The  main usage will be for data normalized by
:ref:`MDNormSCD <algm-MDNormSCD>`.
A 3D box is created for each peak and the background and peak data are separated.  The intensity and sigma of the
intensity is found from the grid inside the peak and the background is subtracted.  The boxes are created and integrated
in parallel and less memory is required than binning all HKL at once.

:math:`I_{corr} = I_{peak} - pts_{peak}/pts_{bg} * I_{bg}`

with the errors summed in quadrature:

:math:`\sigma_{I,corr}^2 = \sigma_{I,peak}^2 + (pts_{peak}/pts_{bg})^2 * \sigma_{I,bg}^2`

Using the DeltaHKL parameter, the problem of nearly peaks or regions of diffuse scattering can be avoided.  Also for
normalized data, the unmeasured data points are excluded from the background. See white regions in last figure.

.. figure:: /images/peak3d.png
   :alt: peak3d.png
   :width: 400px
   :align: center

   Peak Integration Input. 3D Box.

.. figure:: /images/IntegratePeaksMDHKLbox.png
   :alt: IntegratePeaksMDHKLbox.png
   :width: 400px
   :align: center

   Integration slice at center of box.

.. figure:: /images/IntegratePeaksMDHKLpeak.png
   :alt: IntegratePeaksMDHKLpeak.png
   :width: 400px
   :align: center

   Integration slice of peak grid points.

.. figure:: /images/IntegratePeaksMDHKLbkg.png
   :alt: IntegratePeaksMDHKLbkg.png
   :width: 400px
   :align: center

   Integration slice of background grid points.


Usage
-----

**Example - IntegratePeaksMDHKL event histo**

.. testcode:: IntegratePeaksMDHKLExample

   #Create PeaksWorkspace
   sampleWs = CreateSampleWorkspace()
   pws = CreatePeaksWorkspace(InstrumentWorkspace=sampleWs,NumberOfPeaks=3)
   p = pws.getPeak(0)
   p.setHKL(5,0,0)
   p = pws.getPeak(1)
   p.setHKL(0,0,0)
   p = pws.getPeak(2)
   p.setHKL(-5,0,0)
   #Test with MDEventWorkspace
   mdws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names='[H,0,0],[0,K,0],[0,0,L]',Units='A^-1,A^-1,A^-1',Frames='HKL,HKL,HKL')
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,-5,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,0,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,5,0,0,1])
   pws =IntegratePeaksMDHKL(InputWorkspace=mdws,PeaksWorkspace=pws,DeltaHKL=1.5,GridPoints=21)
   for i in range(3):
       p = pws.getPeak(i)
       print('{:.7f} {:.9f}'.format(p.getIntensity(),p.getSigmaIntensity()))
   #Test with MDHistoWorkspace
   mdws = BinMD(InputWorkspace=mdws,AlignedDim0="[H,0,0],-10,10,101",AlignedDim1="[0,K,0],-10,10,101",AlignedDim2="[0,0,L],-10,10,101")
   pws =IntegratePeaksMDHKL(InputWorkspace=mdws,PeaksWorkspace=pws,DeltaHKL=1.5,GridPoints=21)
   for i in range(3):
       p = pws.getPeak(i)
       print('{:.7f} {:.9f}'.format(p.getIntensity(),p.getSigmaIntensity()))


Output:

.. testoutput:: IntegratePeaksMDHKLExample

   99965.2588423 316.186057523
   99965.2588423 316.186057523
   99965.2588423 316.186057523
   99959.4727293 316.176064633
   99978.7624499 316.200132638
   99961.9286227 316.179180305

.. categories::

.. sourcelink::
