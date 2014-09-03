.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a hybrid between :ref:`algm-IntegratePeaksMD` and :ref:`algm-IntegratePeaksUsingClusters`. Each peak region is treated as a separate image and rebinned accordingly. The background threshold is automatically determined around each peak, by averaging over all pixels in that region. 
The NumberOfBins and BackgroundOuterRadius are global to all Peaks. The actual background threshold is calculated independently for each peak based on NumberOfBins, BackgroundOuterRadius and the signal values in that region. This algorithm is in general faster than :ref:`algm-IntegratePeaksUsingClusters` and has a better ability to distinguish peaks from the background because each peak is treated independently.

Integrates arbitary shaped single crystal peaks defined on an
:ref:`MDHistoWorkspace <MDHistoWorkspace>` using connected component
analysis to determine regions of interest around each peak of the
`PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_. The output is an integrated
`PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_ as well as a group of images :ref:`WorkspaceGroup <WorkspaceGroup>` of :ref:`MDWorkspaces <MDWorkspace>`  containing the
labels assigned to each cluster for diagnostic and visualisation
purposes.

**The algorithm makes no assmptions about Peak shape or size** and can
therfore be used where integration over defined shapes
:ref:`algm-IntegratePeaksMD` and
:ref:`algm-IntegrateEllipsoids`, for example, will not
work.

.. figure:: /images/ClusterImage.png
   :alt: ClusterImage.png

   Cluster Label region displayed in the SliceViewer. Peak centre is marked with an X. The green circle illustrates the integration region used by :ref:`algm-IntegratePeaksMD`

Warnings and Logging
--------------------

The algorithm will generate warning. There are three main warning to
know about.

Off the Image Edge
##################

The algorithm will warn about unreachable peaks (off the image). This
may be because the peaks detected were off the edge of the detector, or
because the image was cropped in BinMD in such a way that that part of
the detector/TOF space is no longer accessible.

No Cluster Corresponding to Peak
################################

This is because the input `PeaksWorkspace <http://www.mantidproject.org/PeaksWorkspace>`_ has peaks
that do not align with peaks in the image. The error could either be on
the side of the input PeaksWorkspace (spurious peaks), or of the
:ref:`MDHistoWorkspace <MDHistoWorkspace>` generated as part of processing.
One thing to verify is that the combination of Threshold and
Normalization input parameters are not so low that they are treating
genuine peaks in the image as background.

Usage
-----

**Example - Simple Integration of TOPAZ data**

.. testcode:: IntegratePeaksUsingClustersExample

   # Load an MDEventWorkspace (QLab) containing some SC diffration peaks
   mdew = Load("TOPAZ_3680_5_sec_MDEW.nxs")
   # The following algorithms need to know that frame to use, this is an older file. Newer files will automaticall have this.
   SetSpecialCoordinates(InputWorkspace=mdew, SpecialCoordinates='Q (lab frame)')
   # Find the 5 most intense peaks
   peaks = FindPeaksMD(InputWorkspace=mdew, MaxPeaks=5)
   # Perform the integration
   integrated_peaks, cluster_images = IntegratePeaksHybrid(InputWorkspace=mdew, PeaksWorkspace=peaks, BackgroundOuterRadius=0.4)

.. categories::
