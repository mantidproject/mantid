.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Description
-----------

ProjectMD reduces the dimensionality of a MHDistoWorkspace by summing it
along a specified dimension. Example: you have a 3D MDHistoWorkspace
with X,Y,TOF. You sum along Z (TOF) and the result is a 2D workspace X,Y
which gives you a detector image.

Besides the obvious input and output workspaces you have to specify the
dimension along which you wish to sum. The following code is used:

X
    Dimension 0
Y
    Dimension 1
Z
    Dimension 2
K
    Dimension 3

The summation range also has to be specified. This is in indices into
the appropriate axis.

.. categories::
