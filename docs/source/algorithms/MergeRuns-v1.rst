.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Combines the data contained in an arbitrary number of input workspaces.
If the input workspaces do not have common binning, the bins in the
output workspace will cover the entire range of all the input
workspaces, with the largest bin widths used in regions of overlap.

The combination of each workspace is performed using the :ref:`algm-Plus` algorithm,
this does not perform any weighting based on the duration of collection, or proton charge.
If you wish to perform Merge runs that should not be equally weighted then they should be
corrected individually prior to merging.

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Group workspaces will be merged respecting the periods within each
group. For example if you have two multiperiod group workspaces A and B
and an output workspace C. A contains matrix workspaces A\_1 and A\_2,
and B contains matrix workspaces B\_1 and B2. Since this is multiperiod
data, A\_1 and B\_1 share the same period, as do A\_2 and B\_2. So
merging must be with respect to workspaces of equivalent periods.
Therefore, merging is conducted such that A\_1 + B\_1 = C\_1 and A\_2 +
B\_2 = C\_2.

Group Workspaces that are not multiperiod
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If group workspaces are provided that are not multi-period, this
algorithm will merge across all nested workspaces, to give a single
output matrix workspace.

Merging Sample Logs
###################

Sample logs are optionally merged when running this algorithm. The behaviour
when merging is defined in the instrument parameter file, but can be added to
or overridden via this algorithm. Definitions in the XML file are given as
shown in the example below. See the usage examples at the end of this document
for examples of overriding the behaviour defined in the XML file.

When performing the tolerance check for the warn or fail options it is always
with respect to the first workspace in the merge. When choosing via the GUI
this will be the first workspace that was selected.

**Note:** this currently only works when the underlying workspaces being merged are
Matrix Workspaces.

**Note:** The sample log in the merged workspace is replaced in the case of Sum,
Time Series and List merges. This means these merge types can not be used together
for a given sample log.

.. rstcheck: ignore-next-code-block
.. code-block:: xml

    <parameter name="sample_logs_sum" type="string">
        <value val="duration, monitor1.monsum" />
    </parameter>
    <parameter name="sample_logs_time_series" type="string">
        <value val="sample.temperature, sample.pressure" />
    </parameter>
    <parameter name="sample_logs_list" type="string">
        <value val="run_number" />
    </parameter>
    <parameter name="sample_logs_warn" type="string">
        <value val="EPP, Fermi.phase, sample.temperature" />
    </parameter>
    <parameter name="sample_logs_warn_tolerances" type="string">
        <value val="5, 0.001, 50" />
    </parameter>
    <parameter name="sample_logs_fail" type="string">
        <value val="experiment_identifier, Ei, Fermi.rotation_speed" />
    </parameter>
    <parameter name="sample_logs_fail_tolerances" type="string">
        <value val="0, 0.1, 2" />
    </parameter>

Merging Workspaces with Detector Scans
######################################

If the workspaces being merged contain detector scans then there are currently two options:

1. The workspaces have identical scan intervals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this case the workspaces will be merged as they normally would be by MergeRuns,
that is the counts in the two workspaces are summed. The detectors must have the same
positions, rotations etc. for all time intervals, else the algorithm will throw.

2. The workspaces have different scan intervals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For this case the scan intervals must not overlap. The merged workspace histograms are
appended to the end of the first workspace.


ChildAlgorithms used
####################

The :ref:`algm-Rebin` algorithm is used, if necessary, to put all the
input workspaces onto a common binning.

The :ref:`algm-Plus` algorithm is used to combine each of the workspaces
together one pair at a time.

Usage
-----

**Example: Merge Two Workspaces**

.. testcode:: ExWs

   dataX = [1, 2, 3, 4, 5]
   dataY = [6, 15, 21, 9]

   a = CreateWorkspace(dataX, dataY)
   b = CreateWorkspace(dataX, dataY)

   merged = MergeRuns(InputWorkspaces="a, b")

   print("a      = {}".format(a.readY(0)))
   print("b      = {}".format(b.readY(0)))
   print("merged = {}".format(merged.readY(0)))

Output:

.. testoutput:: ExWs

   a      = [ 6. 15. 21.  9.]
   b      = [ 6. 15. 21.  9.]
   merged = [12. 30. 42. 18.]

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

   print("group_1 = [{},".format(group_1[0].readY(0)))
   print("           {}]".format(group_1[1].readY(0)))

   print("group_2 = [{},".format(group_2[0].readY(0)))
   print("           {}]".format(group_2[1].readY(0)))

   print("merged   = {}".format(merged.readY(0)))

Output:

.. testoutput:: ExWsGroup

   group_1 = [[ 6. 15. 21.  9.],
              [ 6. 15. 21.  9.]]
   group_2 = [[ 6. 15. 21.  9.],
              [ 6. 15. 21.  9.]]
   merged   = [24. 60. 84. 36.]

.. include:: ../usagedata-note.txt

**Example: Merge Workspace Summing Sample Logs**

.. testcode:: MergeSampleLogs

  Load(Filename='MUSR00015189.nxs, MUSR00015190.nxs', OutputWorkspace='gws')

  merged = MergeRuns(InputWorkspaces='MUSR00015189_1, MUSR00015190_1',
                     SampleLogsSum='dur')

  print(merged.run().getLogData('dur').value)

Output:

.. testoutput:: MergeSampleLogs

  200.0

**Example: Merge Workspace Combining Sample Logs as a TimeSeries**

.. testcode:: MergeSampleLogs

  Load(Filename='MUSR00015189.nxs, MUSR00015190.nxs', OutputWorkspace='gws')

  merged = MergeRuns(InputWorkspaces='MUSR00015189_1, MUSR00015190_1',
                     SampleLogsTimeSeries='sample_magn_field')

  print(merged.run().getLogData('sample_magn_field').valueAsString().rstrip())

Output:

.. testoutput:: MergeSampleLogs

  2007-Nov-27 17:10:35  1350
  2007-Nov-27 17:12:30  1360

**Example: Merge Workspace Combining Sample Logs as a List**

.. testcode:: MergeSampleLogs

  Load(Filename='MUSR00015189.nxs, MUSR00015190.nxs', OutputWorkspace='gws')

  merged = MergeRuns(InputWorkspaces='MUSR00015189_1, MUSR00015190_1',
                     SampleLogsList='sample_magn_field')

  print(merged.run().getLogData('sample_magn_field').value)

Output:

.. testoutput:: MergeSampleLogs

  1350, 1360

**Example: Merge Workspace Combining Sample Logs with a Warnining if Different**

.. testcode:: MergeSampleLogs

  Load(Filename='MUSR00015189.nxs, MUSR00015190.nxs', OutputWorkspace='gws')

  merged = MergeRuns(InputWorkspaces='MUSR00015189_1, MUSR00015190_1',
                     SampleLogsTimeSeries='sample_temp',
                     SampleLogsWarn='sample_magn_field')

  print(merged.run().getLogData('sample_temp').size())

Output:

.. testoutput:: MergeSampleLogs

  2

**Example: Merge Workspace Combining Sample Logs with an Error if Different**

.. testcode:: MergeSampleLogs

  Load(Filename='MUSR00015189.nxs, MUSR00015190.nxs', OutputWorkspace='gws')

  merged = MergeRuns(InputWorkspaces='MUSR00015189_1, MUSR00015190_1',
                     SampleLogsTimeSeries='sample_temp',
                     SampleLogsFail='sample_magn_field, nspectra',
                     SampleLogsFailTolerances='5, 0')

  print(merged.run().getLogData('sample_temp').size())

Output:

.. testoutput:: MergeSampleLogs

  1

Related Algorithms
------------------
:ref:`ConjoinXRuns <algm-ConjoinXRuns>` concatenates the workspaces in the x dimension by handling the merging of the Sample Logs.
:ref:`ConjoinWorkspaces <algm-ConjoinWorkspaces>` combines workspaces by appending their spectra.

.. categories::

.. sourcelink::
