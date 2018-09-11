.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm takes an image workspace (a.k.a
`IMDHistoWorkspace <http://www.mantidproject.org/IMDHistoWorkspace>`_) and determines the faces of
the clusters contained within the image. The image is expected to be a
labeled image workspace outputted from
:ref:`algm-IntegratePeaksUsingClusters`. The
algorithm generates a `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ as output,
which contains all the cluster edge faces required to draw the outer
edge of all clusters within the workspace.

You may optionally provide a FilterWorkspace, which is a
:ref:`PeaksWorkspace <PeaksWorkspace>`. If provided, the Peak locations are
projected onto the InputWorkspace and the center locations are used to
restrict the output to only include the clusters that are the union
between the peak locations and the image clusters.

If LimitRows is set to True (default), then you may specify a maximum
number of rows to report. If the algorithm generates more rows that the
MaximumRows that you set, then it will emit a warning, and also, set the
TruncatedOutput output property to false.

.. categories::

.. sourcelink::
