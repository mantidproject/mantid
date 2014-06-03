.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Integrates arbitary shaped single crystal peaks defined on an
`MDHistoWorkspace <MDHistoWorkspace>`__ using connected component
analysis to determine regions of interest around each peak of the
`PeaksWorkspace <PeaksWorkspace>`__. The output is an integrated
`PeaksWorkspace <PeaksWorkspace>`__ as well as an image containing the
labels assigned to each cluster for diagnostic and visualisation
purposes.

**The algorithm makes no assmptions about Peak shape or size** and can
therfore be used where integration over defined shapes
:ref:`_algm-IntegratePeaksMD` and
:ref:`_algm-IntegrateEllipsoids`, for example, will not
work.

.. figure:: /images/ClusterImage.png
   :alt: ClusterImage.png

   ClusterImage.png
*Cluster Label region displayed in the `SliceViewer <SliceViewer>`__.
Peak centre is marked with an X. The green circle illustrates the
integration region used by :ref:`_algm-IntegratePeaksMD`*

A threshold for the Peak should be defined below which, parts of the
image are treated as background. The normalization method in combination
with the threshold may both be used to define a background. We suggest
keeping the default of VolumeNormalization so that changes in the
effective bin size do not affect the background filtering.

This algorithm uses an imaging technique, and it is therefore important
that the MDHistoWorkspace you are using is binned to a sufficient
resolution via :ref:`_algm-BinMD`. You can overlay the intergrated peaks
workspace in the `Slice
Viewer <MantidPlot:_SliceViewer#Viewing_Peaks_Workspaces>`__ over the
generated Cluster Labeled OutputWorkspaceMD to see what the interation
region used for each peak amounts to.

Notes for running
-----------------

It is suggested that you **initially run the algorithm on a coarse
image**. This will help you tune the Threshold parameters. The algorithm
generates a large memory footprint, so it is suggested that you keep the
initial image small, and run on hardware with sufficient memory to store
multiple workspace of equal size to the input MDWorkspace (generated as
part of the connected component analysis).

Warnings and Logging
--------------------

The algorithm will generate warning. There are three main warning to
know about.

Off the Image Edge
~~~~~~~~~~~~~~~~~~

The algorithm will warn about unreachable peaks (off the image). This
may be because the peaks detected were off the edge of the detector, or
because the image was cropped in BinMD in such a way that that part of
the detector/TOF space is no longer accessible.

No Cluster Corresponding to Peak
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is because the input `PeaksWorkspace <PeaksWorkspace>`__ has peaks
that do not align with peaks in the image. The error could either be on
the side of the input PeaksWorkspace (spurious peaks), or of the
`MDHistoWorkspace <MDHistoWorkspace>`__ generated as part of processing.
One thing to verify is that the combination of Threshold and
Normalization input parameters are not so low that they are treating
genuine peaks in the image as background.

Multiple Peaks Assigned to the same Cluster
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This means overlapping peaks in the image. This is a problem because
both peaks will be given an integrated value that is the sum of the
entire cluster. You may need to increase the Threshold parameter to
resolve this problem.

For more in-depth analysis, the algorithm will produce debug log
messages.

.. categories::
