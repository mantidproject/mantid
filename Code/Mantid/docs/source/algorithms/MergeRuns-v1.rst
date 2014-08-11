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

**Workspace2Ds**: Each input workspace must have common binning for all
its spectra.

**EventWorkspaces**: This algorithm is Event-aware; it will append
event lists from common spectra. Binning parameters need not be compatible;
the output workspace will use the first workspaces' X bin boundaries.

**WorkspaceGroups**: Each nested has to be one of the above.

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

Usage
-----

**Example: Merge Two Workspaces**

.. testcode:: ExWs

   dataX = [1, 2, 3, 4, 5]
   dataY = [6, 15, 21, 9]

   a = CreateWorkspace(dataX, dataY)
   b = CreateWorkspace(dataX, dataY)

   merged = MergeRuns(InputWorkspaces="a, b")

   print "a      = " + str(a.readY(0))
   print "b      = " + str(b.readY(0))
   print "merged = " + str(merged.readY(0))

.. testoutput:: ExWs

   a      = [  6.  15.  21.   9.]
   b      = [  6.  15.  21.   9.]
   merged = [ 12.  30.  42.  18.]

**Example: Merge Two GroupWorkspaces**

.. testcode:: ExWsGroup

   dataX = [1, 2, 3, 4, 5]
   dataY = [6, 15, 21, 9]

   a = CreateWorkspace(dataX, dataY)
   b = CreateWorkspace(dataX, dataY)
   c = CreateWorkspace(dataX, dataY)
   d = CreateWorkspace(dataX, dataY)

   group_1 = GroupWorkspaces(InputWorkspaces="a, b")
   group_2 = GroupWorkspaces(InputWorkspaces="c, d")

   merged = MergeRuns(InputWorkspaces="group_1, group_2")

   print "group_1 = [" + str(group_1[0].readY(0)) + ","
   print "           " + str(group_1[1].readY(0)) + "]"

   print "group_2 = [" + str(group_2[0].readY(0)) + ","
   print "           " + str(group_2[1].readY(0)) + "]"

   print "merged   = " + str(merged.readY(0))

.. testoutput:: ExWsGroup

   group_1 = [[  6.  15.  21.   9.],
              [  6.  15.  21.   9.]]
   group_2 = [[  6.  15.  21.   9.],
              [  6.  15.  21.   9.]]
   merged   = [ 24.  60.  84.  36.]

.. categories::
