.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm masks a :ref:`MDWorkspace <MDWorkspace>` in-situ.

This algorithm will first clear-out any existing masking and then apply
the new masking.

Simple Example
--------------

Mask as single box region in a 3D workspace with Dimension ids X, Y, Z.
Suppose that the dimensions extended from -2 to 2 in each dimension and
you want to mask the central region.

``MaskMD("Workspace"=workspace,Dimensions="X,Y,Z",Exents="-1,1,-1,1,-1,1")``

Complex Example
---------------

Mask two box regions in a 3D workspace, where the input workspace is the
same as above. Here we attempt to mask two opposite corners of the 3D
workspace.

``MaskMD("Workspace"=workspace,Dimensions="X,Y,Z,X,Y,Z",Extents="-2,-1,-2,-1,-2,-1,+1,+2,+1,+2,+1,+2")``

In this example, because the dimensionality is 3 and because 6 dimension
ids have been provided, the algorithm treats {X,Y,Z} as one masking
region and the following {X,Y,Z} as another. Likewise of the 12 extents
inputs provided; the first 6 entries {-2,-1,-2,-1,-2,-1} are min/max
values for the first {X,Y,Z} and the latter 6 {+1,+2,+1,+2,+1,+2} relate
to the last {X,Y,Z}. Applying this masking will result in two completely
separate areas masked in a single call to the algorithm.

.. categories::
