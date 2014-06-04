.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm merges several `MDWorkspaces <MDWorkspace>`__ together
into one by adding their events together.

The algorithm starts by going through the list of
`MDWorkspaces <MDWorkspace>`__ to find the extents that fully encompass
all input workspaces in each dimension. The number and names of
dimensions must match for all input workspaces.

The output workspace is created with these dimensions and the box
parameters specified above. Then the events from each input workspace
are appended to the output.

See also: :ref:`algm-MergeMDFiles`, for merging when system
memory is too small to keep the entire workspace in memory.

.. categories::
