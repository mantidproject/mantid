.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Applies detector grouping to a workspace. (Muon version).

Expect the DetectorGroupingTable to contain one column only. It should
be of type vector\_int (std::vector). Every row corresponds to a group,
and the values in the only column are IDs (not indices!) of the
detectors which spectra should be contained in the group. Name of the
column is not used.

One detector might be in more than one group. Empty groups are ignored.
std::invalid\_argument exceptions are thrown if table format is not
correct, there are no non-empty groups or one of the detector IDs does
not exist in the workspace.

.. categories::
