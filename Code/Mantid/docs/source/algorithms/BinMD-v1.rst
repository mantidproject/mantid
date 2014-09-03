.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs dense binning of the events in multiple
dimensions of an input :ref:`MDWorkspace <MDWorkspace>` and
places them into a dense :ref:`MDHistoWorkspace <MDHistoWorkspace>` with 1-4 dimensions.

The input :ref:`MDWorkspace <MDWorkspace>` may have more dimensions than the number of
output dimensions. The names of the dimensions in the DimX, etc.
parameters are used to find the corresponding dimensions that will be
created in the output.

An ImplicitFunction can be defined using the ImplicitFunctionXML
parameter; any points NOT belonging inside of the ImplicitFunction will
be set as NaN (not-a-number).

Axis-Aligned Binning
####################

This is binning where the output axes are aligned with the original
workspace. Specify each of the AlignedDim0, etc. parameters with these
values, separated by commas:

-  First, the name of the dimension in the original workspace
-  Next, the start and end position along that dimension
-  Finally, the number of bins to use in that dimensions.

If you specify fewer output dimensions, then events in the remaining
dimensions will be integrated along all space. If you wish to integrate
only within a range, then specify the start and end points, with only 1
bin.

Non-Axis Aligned Binning
########################

This allows rebinning to a new arbitrary space, with rotations,
translations, or skewing. This is given by a set of basis vectors and
other parameters

The **BasisVector0...** parameters allow you to specify the basis
vectors relating the input coordinates to the output coordinates. They
are string with these parameters, separated by commas: 'name, units,
x,y,z,..,':

-  Name: of the new dimension
-  Units: string giving the units
-  x, y, z, etc.: a vector, matching the number of dimensions, giving
   the direction of the basis vector

The **Translation** parameter defines the translation between the
spaces. The coordinates are given in the input space dimensions, and
they correspond to 0,0,0 in the output space.

The **OutputExtents** parameter specifies the min/max coordinates *in
the output coordinate* space. For example, if you the output X to range
from -5 to 5, and output Y to go from 0 to 10, you would have: "-5,5,
0,10".

The **OutputBins** parameter specifies how many bins to use in the
output workspace for each dimension. For example, "10,20,30" will make
10 bins in X, 20 bins in Y and 30 bins in Z.

If the **NormalizeBasisVectors** parameter is **True**, then the
distances in the *input* space are the same as in the *output* space (no
scaling).

-  For example, if BasisVector0=(1,1), a point at (1,1) transforms to
   :math:`x=\sqrt{2}`, which is the same as the distance in the input
   dimension.

If the **NormalizeBasisVectors** parameter is **False**, then the
algorithm will take into account the *length* of each basis vector.

-  For example, if BasisVector0=(1,1), a point at (1,1) transforms to
   :math:`x=1`. The distance was scaled by :math:`1/\sqrt{2}`

Finally, the **ForceOrthogonal** parameter will modify your basis
vectors if needed to make them orthogonal to each other. Only works in 3
dimensions!

Binning a MDHistoWorkspace
##########################

It is possible to rebin a :ref:`MDHistoWorkspace <MDHistoWorkspace>`. Each
:ref:`MDHistoWorkspace <MDHistoWorkspace>` holds a reference to the
:ref:`MDWorkspace <MDWorkspace>` that created it, as well as the
coordinate transformation that was used. In this case, the rebinning is
actually performed on the original :ref:`MDWorkspace <MDWorkspace>`, after suitably
transforming the basis vectors.

Only the non-axis aligned binning method can be performed on a
MDHistoWorkspace! Of course, your basis vectors can be aligned with the
dimensions, which is equivalent.

For more details on the coordinate transformations applied in this case,
please see `BinMD Coordinate
Transformations <http://www.mantidproject.org/BinMD_Coordinate_Transformations>`__.

.. figure:: /images/BinMD_Coordinate_Transforms_withLine.png
   :alt: BinMD_Coordinate_Transforms_withLine.png

.. categories::
