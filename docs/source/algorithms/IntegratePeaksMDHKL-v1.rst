
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

:ref:`IntegratePeaksMDHKL <algm-IntegratePeaksMDHKL>` provides integration of a 
:ref:`MDHistoWorkspace <MDHistoWorkspace>` or :ref:`MDEventWorkspace <MDEventWorkspace>` in 3-dimensions. 
The units of the workspace must be HKL.  The  main usage will be for data normalized by :ref:`MDNormSCD <algm-MDNormSCD>`.
A 3D box is created for each peak and the background and peak data are separated.  The intensity and sigma of the 
intensity is found from the grid inside the peak and the background is subtracted.  The boxes are created and integrated 
in parallel and less memory is required than binning all HKL at once.


.. figure:: /images/peak3d.png
   :alt: peak3d.png
   :width: 400px
   :align: center
   
   Peak Integration Input. 3D Box.
   
.. figure:: /images/IntegratePeaksMDHKLbox.png
   :alt: IntegratePeaksMDHKLbox.png
   :width: 400px
   :align: left

.. figure:: /images/IntegratePeaksMDHKLpeak.png
   :alt: IntegratePeaksMDHKLpeak.png
   :width: 400px
   :align: center

.. figure:: /images/IntegratePeaksMDHKLbkg.png
   :alt: IntegratePeaksMDHKLbkg.png
   :width: 400px
   :align: right
   
   Integration Output. Slice at center of box (left) with peak (center) grid point and background (right) points separated.
   

Usage
-----

**Example - IntegratePeaksMDHKL simple cut**

.. testcode:: IntegratePeaksMDHKLExample

   mdws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names='[H,0,0],[0,K,0],[0,0,L]',Units='U,U,U')
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,-5,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,0,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,5,0,0,1])
   pws = CreatePeaksWorkspace(mdws, NumberOfPeaks=0)
   # Add peak to the peaks workspace
   AddPeak( pws, ws, TOF=100, DetectorID=101, Height=1 )

   #Integrate out 2 dimensions
   #pws =IntegratePeaksMDHKL(InputWorkspace=mdws,PeaksWorkspace=pws)


Output:

.. testoutput:: IntegratePeaksMDHKLExample


