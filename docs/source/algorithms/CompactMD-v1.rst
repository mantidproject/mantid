.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
Used to crop an n-dimensional :ref:`MDHistoWorkspace <MDHistoWorkspace>` to the first non-zero signal values found in all dimensions.

Cropping
--------
The cropping is done by supplying `IntegrateMDHistoWorkspace <http://docs.mantidproject.org/nightly/algorithms/IntegrateMDHistoWorkspace-v1.html>`__ with the minimum and maximum extents associated with the first non-zero
signal values in the workspace.


Usage
-----


**Example - CompactMD on MDHistoWorkspace**

.. testcode:: CompactMDOnMDHistoWorkspace

    import math
    #create an MDEventWorkspace for Rebinning
    mdws = CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', Names='A,B,C', Units='U,U,U')
    FakeMDEventData(InputWorkspace=mdws, PeakParams='100000,-5,-5,0,1')
    FakeMDEventData(InputWorkspace=mdws, PeakParams='100000,0,0,0,1')
    FakeMDEventData(InputWorkspace=mdws, PeakParams='100000,5,5,0,1')
    #Rebin mdws to create an MDHistoWorkspace
    binned_ws = BinMD(InputWorkspace=mdws, AxisAligned=False, BasisVector0='a,unit,1,1,0',BasisVector1='b,unit,-1,1,0',BasisVector2='c,unit,0,0,1',NormalizeBasisVectors=True,Translation=[-10,-10,0], OutputExtents=[0,math.sqrt(2*20*20),-2,2,-10,10], OutputBins=[100, 100, 1] )
    
    #A visualisation of the rebinned_ws can be found in the 'Input' section below.
    
    #run CompactMD on the rebinned workspace 
    compact_output = CompactMD(binned_ws)
    
    #A visualisation of the compacted workspace can be found in the 'Output' section below.

Input:

.. figure:: /images/RebinnedWorkspaceNoCompactMDApplied.png
   :alt: RebinnedWorkspaceNoCompactMDApplied.png
   :width: 400px
   :align: center

   
Output:

.. figure:: /images/RebinnedWorkspaceWithCompactMDApplied.png
   :alt: RebinnedWorkspaceWithCompactMDApplied.png
   :width: 400px
   :align: center

   
   
.. categories::

.. sourcelink::