.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

If the two input workspaces have the same number of histograms, each histogram
in WorkspaceToRebin is rebinned so that its bin edges match the histogram with
the same workspace index in WorkspaceToMatch.

If the two input workspaces have different numbers of histograms, each histogram
in WorkspaceToRebin is rebinned to match the first histogram in WorkspaceToMatch.

The algorithm uses the same code as the :ref:`algm-Rebin` algorithm, to actually
do the work.

.. categories::

.. sourcelink::
