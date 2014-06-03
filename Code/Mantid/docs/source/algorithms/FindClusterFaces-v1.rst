.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Algorithm takes an image workspace (a.k.a
`IMDHistoWorkspace <IMDHistoWorkspace>`__) and determines the faces of
the clusters contained within the image. The image is expected to be a
labeled image workspace outputted from
:ref:`algm-IntegratePeaksUsingClusters`. The
algorithm generates a `TableWorkspace <TableWorkspace>`__ as output,
which contains all the cluster edge faces required to draw the outer
edge of all clusters within the workspace.

You may optionally provide a FilterWorkspace, which is a
`PeaksWorkspace <PeaksWorkspace>`__. If provided, the Peak locations are
projected onto the InputWorkspace and the center locations are used to
restrict the output to only include the clusters that are the union
between the peak locations and the image clusters.

If LimitRows is set to True (default), then you may specify a maximum
number of rows to report. If the algorithm generates more rows that the
MaximumRows that you set, then it will emit a warning, and also, set the
TruncatedOutput output property to false.

.. categories::
