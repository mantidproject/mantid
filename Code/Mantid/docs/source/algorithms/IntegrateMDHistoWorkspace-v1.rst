
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Provides limited integration of a :ref:`MDHistoWorkspace <MDHistoWorkspace>` in n-dimensions. Integration is always axis-aligned. Dimensions can only be integrated out, but no finer rebinning is permitted. Dimensions that do not require further rebinning will be left intact provided that the the binning parameters for those dimensions are not specified. For dimensions that are integrated, limits should be provided to give the range of the data to keep.

The *P1Bin* corresponds to the first dimension of the MDHistoWorkspace, *P2Bin* to the second and so on. *P1Bin=[-1, 1]* indicates that we will integrate this dimension between -1 and 1. *P1Bins=[]* indicates that the shape of this dimension should be unchanged from the input.

.. figure:: /images/PreIntegrateMD.png
   :alt: PreIntegrateMD.png
   :width: 400px
   :align: center
   
   Integration Input. 3D.
   
.. figure:: /images/IntegrateMD.png
   :alt: IntegrateMD.png
   :width: 400px
   :align: center
   
   Integration Output. 2nd and 3rd dimensions integrated out. 
   

Usage
-----

**Example - IntegrateMDHistoWorkspace**

.. testcode:: IntegrateMDHistoWorkspaceExample

   mdws = CreateMDWorkspace(Dimensions=3, Extents=[-10,10,-10,10,-10,10], Names='A,B,C',Units='U,U,U')
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,-5,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,0,0,0,1])
   FakeMDEventData(InputWorkspace=mdws, PeakParams=[100000,5,0,0,1])
   #Histogram to give 3 unintegrated dimensions
   high_d_cut =CutMD(InputWorkspace=mdws, P1Bin=[-10, 0.1, 10], P2Bin=[-10, 0.1, 10], P3Bin=[-10, 0.1, 10], NoPix=True)
   #Integrate out 2 dimensions
   low_d_cut =IntegrateMDHistoWorkspace(InputWorkspace=high_d_cut, P1Bin=[], P2Bin=[-2,2], P3Bin=[-5,5])

   non_integrated_dims = low_d_cut.getNonIntegratedDimensions()
   print 'Number of non integrated dimensions after integration are %i'  % len(non_integrated_dims)
   for dim in non_integrated_dims:
       print 'Non integrated dimension is %s' % dim.getName()
       print 'Limits are from %0.2f to %0.2f' % (dim.getMinimum(), dim.getMaximum())

Output:

.. testoutput:: IntegrateMDHistoWorkspaceExample

  Number of non integrated dimensions after integration are 1
  Non integrated dimension is ['zeta', 0, 0]
  Limits are from -10.00 to 10.00

.. categories::

