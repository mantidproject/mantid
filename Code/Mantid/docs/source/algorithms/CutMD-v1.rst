.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs slicing of multiDimensional data according to a chosen
projection, limits and binning steps. 

The synax is similar to that used by `Horace <http://horace.isis.rl.ac.uk/Manipulating_and_extracting_data_from_SQW_files_and_objects#cut_sqw>`__.

Unlike most Mantid algorithms, CutMD can accept a list of workspaces as the
input workspace, given as the name of a workspace in the analysis data service
or the path to a workspace, or simply a workspace object in python. These will
all then be processed sequentially with the same options. The only requirement
is that the same number of output workspaces are also given so that CutMD knows
what to call each output workspace created.

MDEventWorkspaces
~~~~~~~~~~~~~~~~~~~

For input of type :ref:`MDEventWorkspace <MDWorkspace>` the algorithm uses :ref:`algm-BinMD` or
:ref:`algm-SliceMD` to achieve the binning of the data. The choice of child
algorithm used for slicing in this case is controlled by the NoPix option. 

MDHistoWorkspaces
~~~~~~~~~~~~~~~~~~~

If the input is an :ref:`MDHistoWorkspace <MDHistoWorkspace>` :ref:`algm-BinMD` and :ref:`algm-SliceMD` are not made available as they needto get hold of the original MDEvents associated with an :ref:`MDEventWorkspace <MDWorkspace>` in order to perform the rebinning. As this information is missing from the MDHistoWorkspace images, those operations are forbidden. Instead, a limited subset of the operations are allowed, and are performed via :ref:`algm-IntegrateMDHistoWorkspace`. In this case, the Projection and NoPix properties are ignored. See :ref:`algm-IntegrateMDHistoWorkspace` for how the binning parameters are used.


Creating Projections
~~~~~~~~~~~~~~~~~~~~

Projections are used by CutMD to transform the multidimensional data prior to
cutting it. Projections are provided to CutMD in the form of a :ref:`TableWorkspace <Table Workspaces>`.
The format of these workspaces is as follows:

+------------+--------+-------------------------------------------------------+
| Column     | Type   | Purpose                                               |
+============+========+=======================================================+
| name       | string | Specifies the dimension the row controls. Can be 'u', |
|            |        | 'v', or 'w'.                                          |
+------------+--------+-------------------------------------------------------+
| value      | V3D    | A 3 dimensional vector specifying the axis for the    |
|            |        | dimension. Example: [1,-1,0]                          |
+------------+--------+-------------------------------------------------------+
| offset     | double | The offset value to use for this dimension.           |
+------------+--------+-------------------------------------------------------+
| type       | string | The type/unit of this dimension. 'r' is RLU, 'a' is   |
|            |        | inverse angstroms.                                    |
+------------+--------+-------------------------------------------------------+

A projection table should have three rows: one for u, one for v, and one for w.

There is a helper class called Projection that can be used to construct these
projection tables for you automatically. For example:

.. code-block:: python

   from mantid.api import Projection
   # Create an identity projection
   proj_id = Projection([1,0,0], [0,1,0], [0,0,1])

   # Automatically infer third dimension as being orthogonal to the first two
   proj_rot = Projection([1,1,0], [1,-1,0])

   # Set other properties
   proj_prop = Projection()
   proj_prop.setOffset(0, 100) # Set u offset to 100
   proj_prop.setOffset(1, -5.0) # Set v offset to -5
   proj_prop.setType(1, 'a') # Set v type to be RLU
   proj_prop.setType(2, 'a') # Set w type to be inverse angstroms

   #Create table workspaces from these projections
   ws_id = proj_id.createWorkspace() # Named ws_id
   proj_rot.createWorkspace(OutputWorkspace="ws_rot") # Name ws_rot


When calling createWorkspace inside of algorithms like CutMD, the
OutputWorkspace name must be provided, or createWorkspace will not know what to
call the created workspace:

.. code-block:: python

   #Good:
   CutMD(..., Projection=proj.createWorkspace(OutputWorkspace='proj_ws'), ...)

   #Bad:
   CutMD(..., Projection=proj.createWorkspace(), ...)


Usage
-----

**Example - Contrived example using projections:**

.. testcode:: Example4D

   from mantid.api import Projection

   to_cut = CreateMDWorkspace(Dimensions=4, Extents=[-1,1,-1,1,-1,1,-10,10], Names="H,K,L,E", Units="U,U,U,V")
   # Add two fake peaks so that we can see the effect of the basis transformation

   FakeMDEventData(InputWorkspace='to_cut', PeakParams=[10000,-0.5,0,0,0,0.1])

   FakeMDEventData(InputWorkspace='to_cut', PeakParams=[10000,0.5,0,0,0,0.1])
    
   SetUB(Workspace=to_cut, a=1, b=1, c=1, alpha=90, beta=90, gamma=90)
   SetSpecialCoordinates(InputWorkspace=to_cut, SpecialCoordinates='HKL')

   #Since we only specify u and v, w is automatically calculated to be the cross product of u and v
   projection = Projection([1,1,0], [-1,1,0])
   proj_ws = projection.createWorkspace()
   
   # Apply the cut (PBins property sets the P1Bin, P2Bin, etc. properties for you)
   out_md = CutMD(to_cut, Projection=proj_ws, PBins=([0.1], [0.1], [0.1], [-5,5]), NoPix=True)

   #Another way we can call CutMD:
   #[out1, out2, out3] = CutMD([to_cut, "some_other_file.nxs", "some_workspace_name"], ...)

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
   
**Example - CutMD on MDHistoWorkspace:**

.. testcode:: ExampleMDHisto

   signal  = [1.0] * 100;
   error=[1.0] * 100;
   # Create Histo workspace
   histo_ws=CreateMDHistoWorkspace(Dimensionality=2,Extents=[-10,10,-10,10],SignalInput=signal ,ErrorInput=error, NumberOfBins=[10,10], Names='X,Y', Units='Q,Q')
              
   # Cut the MDHistoWorkspace to give a single bin containing half the data              
   cut= CutMD(InputWorkspace=histo_ws, PBins=[[-10, 10], [-5, 5]]) 

   print 'Total signal in input = %0.2f' %  sum(signal)
   print 'Half the volume should give half the signal = %0.2f' % cut.getSignalArray()

Output:

.. testoutput:: ExampleMDhisto

   Total signal in input = 100.00
   Half the volume should give half the signal = 50.00
   
.. categories::
