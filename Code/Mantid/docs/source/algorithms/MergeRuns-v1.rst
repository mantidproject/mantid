.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Combines the data contained in an arbitrary number of input workspaces.
If the input workspaces do not have common binning, the bins in the
output workspace will cover the entire range of all the input
workspaces, with the largest bin widths used in regions of overlap.

Restrictions on the input workspace
###################################

The input workspaces must contain histogram data with the same number of
spectra and matching units and instrument name in order for the
algorithm to succeed.

**For `Workspace2Ds <http://www.mantidproject.org/Workspace2D>`_**: Each input workspace must have
common binning for all its spectra.

**For `EventWorkspaces <http://www.mantidproject.org/EventWorkspace>`_**: This algorithm is
Event-aware; it will append event lists from common spectra. Binning
parameters need not be compatible; the output workspace will use the
first workspaces' X bin boundaries.

**For `WorkspaceGroups <http://www.mantidproject.org/WorkspaceGroup>`_**: Each nested has to be one
of the above.

Other than this it is currently left to the user to ensure that the
combination of the workspaces is a valid operation.

Processing Group Workspaces
###########################

Multi-period Group Workspaces
#############################

Group workspaces will be merged respecting the periods within each
group. For example if you have two multiperiod group workspaces A and B
and an output workspace C. A contains matrix workspaces A\_1 and A\_2,
and B contains matrix workspaces B\_1 and B2. Since this is multiperiod
data, A\_1 and B\_1 share the same period, as do A\_2 and B\_2. So
merging must be with respect to workspaces of equivalent periods.
Therefore, merging is conducted such that A\_1 + B\_1 = C\_1 and A\_2 +
B\_2 = C\_2.

Group Workspaces that are not multiperiod
#########################################

If group workspaces are provided that are not multi-period, this
algorithm will merge across all nested workspaces, to give a singe
output matrix workspace.

ChildAlgorithms used
####################

The :ref:`algm-Rebin` algorithm is used, if neccessary, to put all the
input workspaces onto a common binning.

.. categories::
