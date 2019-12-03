.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that merges down a workspace group
and sums the spectra using weighted mean and Ranges for each
spectra. This is done by executing several sub-algorithms as
listed below.

#. :ref:`algm-Rebin` To rebin all spectra to have common bins.
#. :ref:`algm-ConjoinWorkspaces` repeated for every workspaces in the workspace group.
#. :ref:`algm-MatchSpectra` Matched against the spectra with the largest original x range.
#. :ref:`algm-CropWorkspaceRagged` to cut spectra to match the X limits given.
#. :ref:`algm-Rebin` To rebin all spectra to have common bins.
#. :ref:`algm-SumSpectra` using `WeightedSum=True` and `MultiplyBySpectra=False`.

Workflow
########

.. diagram:: MergeWorkspacesWithLimits-v1_wkflw.dot
