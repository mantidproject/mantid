.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

MDHistoToWorkspace2D flattens a MDHistoWorkspace into a Workspace2D. It
can process MDHistoWorkspaces of any dimensionality. The last dimension
of the MDHistoWorkspace becomes the spectra length. Flattening happens
such that the first dimension of the MDHistoWorkspace is the slowest
varying, the second the second slowest varying and so on.

This tool is useful as many algorithms in Mantid only apply to
Workspace2D. After conversion with MDHistoToWorkspace2D such algorithms
can also be applied to MD data.

.. categories::
