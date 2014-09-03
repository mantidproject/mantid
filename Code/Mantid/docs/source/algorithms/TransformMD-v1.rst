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

.. categories::
