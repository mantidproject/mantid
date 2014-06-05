.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Performs 1D stitching of Reflectometry 2D MDHistoWorkspaces. Based on
the Quick script developed at ISIS. This only works on 1D Histogrammed
MD Workspaces.

Scales either the LHS or RHS workspace by some scale factor which, can
be manually specified, or calculated from the overlap.

Calculates the weighted mean values in the overlap region and then
combines the overlap region with the difference of the LHS and RHS
workspaces

.. categories::
