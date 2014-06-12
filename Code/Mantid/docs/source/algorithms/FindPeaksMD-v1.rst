.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to find single-crystal peaks in a
multi-dimensional workspace (`MDEventWorkspace <MDEventWorkspace>`__ or
`MDHistoWorkspace <MDHistoWorkspace>`__). It looks for high signal
density areas, and is based on an algorithm designed by Dennis Mikkelson
for ISAW.

The algorithm proceeds in this way:

-  Sorts all the boxes in the workspace by decreasing order of signal
   density (total weighted event sum divided by box volume).

   -  It will skip any boxes with a density below a threshold. The
      threshold is
      :math:`TotalSignal / TotalVolume * DensityThresholdFactor`.

-  The centroid of the strongest box is considered a peak.
-  The centroid of the next strongest box is calculated.

   -  We look through all the peaks that have already been found. If the
      box is too close to an existing peak, it is rejected. This
      distance is PeakDistanceThreshold.

-  This is repeated until we find up to MaxPeaks peaks.

Each peak created is placed in the output
`PeaksWorkspace <PeaksWorkspace>`__, which can be a new workspace or
replace the old one.

This algorithm works on a `MDHistoWorkspace <MDHistoWorkspace>`__
resulting from the :ref:`algm-BinMD` algorithm also. It works in the
same way, except that the center of each bin is used since the centroid
is not accessible. It may give better results on
`Workspace2D <Workspace2D>`__'s that were converted to
`MDWorkspaces <MDWorkspace>`__.

.. categories::
