.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm that can take a slice out of an original
`MDEventWorkspace <MDEventWorkspace>`__ while preserving all the events
contained therein.

It uses the same parameters as :ref:`_algm-BinMD` to determine a
transformation to make from input->output workspace. The difference is
that :ref:`_algm-BinMD` sums events in a regular grid whereas SliceMD
moves the events into the output workspace, which boxes itself.

Please see :ref:`_algm-BinMD` for a detailed description of the
parameters.

Axis-Aligned Slice
~~~~~~~~~~~~~~~~~~

Events outside the range of the slice are dropped. The new output
MDEventWorkspace's dimensions only extend as far as the limit specified.

Non-Axis-Aligned Slice
~~~~~~~~~~~~~~~~~~~~~~

The coordinates of each event are transformed according to the new basis
vectors, and placed in the output MDEventWorkspace. The dimensions of
the output workspace are along the basis vectors specified.

Splitting Parameters
~~~~~~~~~~~~~~~~~~~~

The **OutputBins** parameter is interpreted as the "SplitInto" parameter
for each dimension. For instance, if you want the output workspace to
split in 2x2x2, you would specify OutputBins="2,2,2".

For 1D slices, it may make sense to specify a SplitInto parameter of 1
in every other dimension - that way, boxes will only be split along the
1D direction.

Slicing a MDHistoWorkspace
~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible to slice a `MDHistoWorkspace <MDHistoWorkspace>`__. Each
MDHistoWorkspace holds a reference to the
`MDEventWorkspace <MDEventWorkspace>`__ that created it, as well as the
coordinate transformation that was used.

In this case, the algorithm is executed on the original
MDEventWorkspace, in the proper coordinates. Perhaps surprisingly, the
output of SliceMD on a MDHistoWorkspace is a MDEventWorkspace!

Only the non-axis aligned slice method can be performed on a
MDHistoWorkspace! Of course, your basis vectors can be aligned with the
dimensions, which is equivalent.

.. categories::
