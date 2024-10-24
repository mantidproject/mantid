.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. warning::

   This algorithm is deprecated (June-2018). Please, use :ref:`MergeRuns <algm-MergeRuns>` instead.

Description
-----------

This algorithm is deprecated and will be removed in the next release. Please use :ref:`algm-MergeRuns` instead.

Merges workspaces from a given list using :ref:`MergeRuns <algm-MergeRuns>` algorithm. Sample logs are merged in the following way.

+---------++-------------------------------+
| Type of || Parameter                     |
| merging ||                               |
+=========++===============================+
| Average || temperature                   |
+---------++-------------------------------+
| Minimum || run_start                     |
+---------++-------------------------------+
| Maximum || run_end                       |
+---------++-------------------------------+
| Summed  || duration, monitor_counts      |
+---------++-------------------------------+
| Listed  || run_number                    |
+---------++-------------------------------+

Other sample logs are copied from the first workspace.

**Valid input workspaces**

Algorithm accepts both, matrix workspaces and groups of matrix workspaces. Valid input workspaces

- must have following sample logs: *channel_width*, *chopper_ratio*, *chopper_speed*, *Ei*, *wavelength*, *full_channels*, *EPP*, *monitor_counts*, *duration*, *run_number*, *run_start*, *run_end*
- must have identical following sample logs: *channel_width*, *chopper_ratio*, *chopper_speed*, *Ei*, *wavelength*, *full_channels*, *EPP*. Tolerance for double comparison is 0.01.
- must have common binning for all its spectra for each input workspace.

If these conditions are not fulfilled, algorithm terminates with an error message.

Sample log *temperature* is optional. If it is present in some of input workspaces, mean value will be calculated. Otherwise, no *temperature* sample log will be created in the output workspace.

Algorithm will produce warning if
- *temperature* and *run_title* sample logs are not present or different,
- some of input workspaces have zero monitor counts.

Usage
-----

**Example - Merge list of workspaces**

.. testcode:: ExTOFTOFMergeRuns2ws

    ws1 = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    ws2 = LoadMLZ(Filename='TOFTOFTestdata.nxs')

    # change sample logs for a second workspace, not needed for real workspaces
    AddSampleLog(ws1, 'temperature', str(294.14), 'Number')
    lognames = 'temperature,run_start,run_end,monitor_counts,run_number'
    logvalues = '296.16,2013-07-28T11:32:19+0053,2013-07-28T12:32:19+0053,145145,TOFTOFTestdata2'
    AddSampleLogMultiple(ws2, lognames, logvalues)

    # Input = list of workspaces
    ws3 = TOFTOFMergeRuns('ws1,ws2')

    # Temperature
    print("Temperature of experiment for 1st workspace (in K): {}".format(ws1.getRun().getLogData('temperature').value))
    print("Temperature of experiment for 2nd workspace (in K): {}".format(ws2.getRun().getLogData('temperature').value))
    print("Temperature of experiment for merged workspaces = average over workspaces (in K): {}".format( ws3.getRun().getLogData('temperature').value))

    # Duration
    print("Duration of experiment for 1st workspace (in s): {}".format(ws1.getRun().getLogData('duration').value))
    print("Duration of experiment for 2nd workspace (in s): {}".format(ws2.getRun().getLogData('duration').value))
    print("Duration of experiment for merged workspaces = sum of all durations (in s): {}".format(ws3.getRun().getLogData('duration').value))

    # Run start
    print("Start of experiment for 1st workspace: {}".format(ws1.getRun().getLogData('run_start').value))
    print("Start of experiment for 2nd workspace: {}".format(ws2.getRun().getLogData('run_start').value))
    print("Start of experiment for merged workspaces = miminum of all workspaces: {}".format(ws3.getRun().getLogData('run_start').value))

    # Run end
    print("End of experiment for 1st workspace: {}".format(ws1.getRun().getLogData('run_end').value))
    print("End of experiment for 2nd workspace: {}".format(ws2.getRun().getLogData('run_end').value))
    print("End of experiment for merged workspaces = maximum of all workspaces: {}".format(ws3.getRun().getLogData('run_end').value))

    # Run number
    print("Run number for 1st workspace: {}".format(ws1.getRun().getLogData('run_number').value))
    print("Run number for 2nd workspace: {}".format(ws2.getRun().getLogData('run_number').value))
    print("Run number for merged workspaces = list of all workspaces: {}".format(ws3.getRun().getLogData('run_number').value))

    # Monitor counts
    print("Monitor counts for 1st workspace: {:.0f}".format(ws1.getRun().getLogData('monitor_counts').value))
    print("Monitor counts for 2nd workspace: {:.0f}".format(ws2.getRun().getLogData('monitor_counts').value))
    print("Monitor counts for merged workspaces = sum over all workspaces: {:.0f}".format(ws3.getRun().getLogData('monitor_counts').value))


Output:

.. testoutput:: ExTOFTOFMergeRuns2ws

    Temperature of experiment for 1st workspace (in K): 294.14
    Temperature of experiment for 2nd workspace (in K): 296.16
    Temperature of experiment for merged workspaces = average over workspaces (in K): 295.15
    Duration of experiment for 1st workspace (in s): 3601
    Duration of experiment for 2nd workspace (in s): 3601
    Duration of experiment for merged workspaces = sum of all durations (in s): 7202
    Start of experiment for 1st workspace: 2013-07-28T10:32:19+0053
    Start of experiment for 2nd workspace: 2013-07-28T11:32:19+0053
    Start of experiment for merged workspaces = miminum of all workspaces: 2013-07-28T10:32:19+0053
    End of experiment for 1st workspace: 2013-07-28T11:32:20+0053
    End of experiment for 2nd workspace: 2013-07-28T12:32:19+0053
    End of experiment for merged workspaces = maximum of all workspaces: 2013-07-28T12:32:19+0053
    Run number for 1st workspace: TOFTOFTestdata
    Run number for 2nd workspace: TOFTOFTestdata2
    Run number for merged workspaces = list of all workspaces: ['TOFTOFTestdata', 'TOFTOFTestdata2']
    Monitor counts for 1st workspace: 136935
    Monitor counts for 2nd workspace: 145145
    Monitor counts for merged workspaces = sum over all workspaces: 282080

**Example - Merge group of workspaces**

.. testcode:: ExTOFTOFMergeRunsGroup

    ws1 = LoadMLZ(Filename='TOFTOFTestdata.nxs')
    ws2 = LoadMLZ(Filename='TOFTOFTestdata.nxs')

    # change sample logs for a second workspace, not needed for real workspaces
    AddSampleLog(ws1, 'temperature', str(294.14), 'Number')
    lognames = 'temperature,run_start,run_end,monitor_counts,run_number'
    logvalues = '296.16,2013-07-28T11:32:19+0053,2013-07-28T12:32:19+0053,145145,TOFTOFTestdata2'
    AddSampleLogMultiple(ws2, lognames, logvalues)

    group=GroupWorkspaces('ws1,ws2')
    groupmerged=TOFTOFMergeRuns(group)

    print("Monitor counts for 1st workspace: {:.0f}".format(ws1.getRun().getLogData('monitor_counts').value))
    print("Monitor counts for 2nd workspace: {:.0f}".format(ws2.getRun().getLogData('monitor_counts').value))
    print("Monitor counts for merged workspaces = sum over all workspaces: {:.0f}".format(groupmerged.getRun().getLogData('monitor_counts').value))

Output:

.. testoutput:: ExTOFTOFMergeRunsGroup

    Monitor counts for 1st workspace: 136935
    Monitor counts for 2nd workspace: 145145
    Monitor counts for merged workspaces = sum over all workspaces: 282080

.. categories::

.. sourcelink::
