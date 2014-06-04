.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Stitches single histogram `Matrix Workspaces <MatrixWorkspace>`__
together outputting a stitched Matrix Workspace. Either the
right-hand-side or left-hand-side workspace can be chosen to be scaled.
Users must provide a Param step (single value), but the binning start
and end are calculated from the input workspaces if not provided.
Likewise, StartOverlap and EndOverlap are optional. If the StartOverlap
or EndOverlap are not provided, then these are taken to be the region of
x-axis intersection.

The workspaces must be histogrammed. Use
:ref:`algm-ConvertToHistogram` on workspaces prior to
passing them to this algorithm.

.. categories::
