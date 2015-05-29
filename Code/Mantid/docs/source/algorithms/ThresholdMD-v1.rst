.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm applies a simple linear transformation to a
:ref:`MDWorkspace <MDWorkspace>` or
:ref:`MDHistoWorkspace <MDHistoWorkspace>`. This could be used, for
example, to scale the Energy dimension to different units.

Each coordinate is tranformed so that :math:`x'_d = (x_d * s_d) + o_d`
where:

-  d : index of the dimension, from 0 to the number of dimensions
-  s : value of the Scaling parameter
-  o : value of the Offset parameter.

You can specify either a single value for Scaling and Offset, in which
case the same m\_scaling or m\_offset are applied to each dimension; or
you can specify a list with one entry for each dimension.

Notes
#####

The relationship between the workspace and the original
:ref:`MDWorkspace <MDWorkspace>`, for example when the MDHistoWorkspace is
the result of :ref:`algm-BinMD`, is lost. This means that you cannot
re-bin a transformed :ref:`MDHistoWorkspace <MDHistoWorkspace>`.

No units are not modified by this algorithm.

Performance Notes
#################

-  Performing the operation in-place (input=output) is always faster
   because the first step of the algorithm if NOT in-place is to clone
   the original workspace.
-  For :ref:`MDHistoWorkspaces <MDHistoWorkspace>` done in-place,
   TransformMD is very quick (no data is modified, just the
   coordinates).
-  For :ref:`MDWorkspaces <MDWorkspace>`, every event's coordinates gets
   modified, so this may take a while for large workspaces.
-  For file-backed :ref:`MDWorkspaces <MDWorkspace>`, you will find much
   better performance if you perform the change in-place (input=output),
   because the data gets written out to disk twice otherwise.

Usage
-----

**Example - Threshold a small workspace:**

.. testcode:: ExThresholdMD

   # Create input workspace
   CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', Names='A,B,C', Units='U,U,U', OutputWorkspace='demo')
   FakeMDEventData(InputWorkspace='demo', PeakParams='32,0,0,0,1')
   threshold_input = BinMD(InputWorkspace='demo', AlignedDim0='A,-2,2,4', AlignedDim1='B,-2,2,4', AlignedDim2='C,-2,2,4')

   # Run the algorithm to set all values greater than 4 to zero
   threshold_output = ThresholdMD(InputWorkspace='threshold_input', Condition='Greater Than', ReferenceValue=4)

   # Print selection before and after
   print "selected bins before threshold greater than 4",threshold_input.getSignalArray()[1,1]
   print "same bins after threshold greater than 4",threshold_output.getSignalArray()[1,1]

Output:

.. testoutput:: ExThresholdMD

   selected bins before threshold greater than 4 [ 0.  4.  5.  0.]
   same bins after threshold greater than 4 [ 0.  4.  0.  0.]

.. categories::
