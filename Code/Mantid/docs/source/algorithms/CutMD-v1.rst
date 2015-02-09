.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs slicing of multiDimensional data according to a chosen projection, and binning choice. The algorithm uses :ref:`algm-BinMD` or 
:ref:`algm-SliceMD` to achieve the binning of the data. The choice of child algorithm used for the slicing is controlled by the NoPix option.

The synax is similar to that used by `Horace <http://horace.isis.rl.ac.uk/Manipulating_and_extracting_data_from_SQW_files_and_objects#cut_sqw>`__.

Usage
-----

**Example - Contrived example using projections:**

.. testcode:: Example4D

   from mantid.api import Projection
   from mantid.kernel import VMD

   to_cut = CreateMDWorkspace(Dimensions=4, Extents=[-1,1,-1,1,-1,1,-10,10], Names="H,K,L,E", Units="U,U,U,V")
   # Add two fake peaks so that we can see the effect of the basis transformation

   FakeMDEventData(InputWorkspace='to_cut', PeakParams=[10000,-0.5,0,0,0,0.1])

   FakeMDEventData(InputWorkspace='to_cut', PeakParams=[10000,0.5,0,0,0,0.1])
    

   SetUB(Workspace=to_cut, a=1, b=1, c=1, alpha=90, beta=90, gamma=90)
   SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')

   #3 dimensional projection
   projection = Projection(3)
   projection.u = VMD( 1,1,0)
   projection.v = VMD(-1,1,0)
   projection.w = VMD( 0,0,1)
   
   # Apply the cut

   out_md = CutMD(to_cut, Projection=projection.toWorkspace(), P1Bin=[0.1], P2Bin=[0.1], P3Bin=[0.1], P4Bin=[-5,5], NoPix=True)
   print 'number of dimensions', out_md.getNumDims()
   print 'number of dimensions not integrated', len(out_md.getNonIntegratedDimensions())
   dim_dE = out_md.getDimension(3)
   print 'min dE', dim_dE.getMaximum()
   print 'max dE', dim_dE.getMinimum()

Output:

.. testoutput:: Example4D

   number of dimensions 4
   number of dimensions not integrated 3
   min dE 5.0
   max dE -5.0

.. categories::
