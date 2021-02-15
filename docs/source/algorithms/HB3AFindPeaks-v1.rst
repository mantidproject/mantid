.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Finds peaks for the given :ref:`MDWorkspace <MDWorkspace>` in Q-space
and sets the UB matrix, refining it for a specific `CellType` and
`Centering`. If the lattice parameters are specified, then the UB
matrix will be found with those parameters by
:ref:`FindUBUsingLatticeParameters
<algm-FindUBUsingLatticeParameters>`. A :ref:`PeaksWorkspace
<PeaksWorkspace>` is created containing the new UB matrix with peaks
indexed and found in Q-space. The UB matrix can be transformed using
:ref:`TransformHKL <algm-TransformHKL>` into the desired form, if for
example a supercell is found.

If multiple data workspaces are provided than the combined peak found
are use to determine the UB matrix, the output with be a
:ref:`WorkspaceGroup <WorkspaceGroup>` with a peaks workspace
corresponding to each input data set.

The input to this algorithm is intended to be DEMAND data that has
been processed from :ref:`HB3AAdjustSampleNorm
<algm-HB3AAdjustSampleNorm>`. See :ref:`HB3AIntegratePeaks
<algm-HB3AIntegratePeaks>` for complete examples of the HB3A workflow.

.. categories::

.. sourcelink::
