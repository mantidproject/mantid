.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is meant to merge a large number of large
MDEventWorkspaces together into one file-backed MDEventWorkspace,
without exceeding available memory.

First, you will need to generate a MDEventWorkspaces NXS file for each
run with a fixed box structure:

-  You can call :ref:`algm-CreateMDWorkspace` with
   MinRecursionDepth = MaxRecursionDepth.

   -  This will make the box structure identical. The number of boxes
      will be equal to SplitInto ^ (NumDims \* MaxRecursionDepth).
   -  Aim for the boxes to be small enough for all events contained to
      fit in memory; without there being so many boxes as to slow down
      too much.

-  This can be done immediately after acquiring each run so that less
   processing has to be done at once.

Then, enter the path to all of the files created previously. The
algorithm avoids excessive memory use by only keeping the events from
ONE box from ALL the files in memory at once to further process and
refine it. This is why it requires a common box structure.

.. seealso:: :ref:`algm-MergeMD`, for merging any MDWorkspaces in system
             memory (faster, but needs more memory).

.. categories::

.. sourcelink::
