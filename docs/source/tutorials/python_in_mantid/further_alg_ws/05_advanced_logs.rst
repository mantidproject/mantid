.. _05_advanced_logs:

==============================
Advanced Access to Sample Logs
==============================


Sample Log Properties
#####################

It can be useful to access different properties from the Sample Logs, or add to them:

.. code-block:: python

    from mantid.simpleapi import *

    # Standard Sample Log set-up
    ws = Load('CNCS_7860_event')
    run = ws.getRun()
    chopper = run.getLogData('ChopperStatus1')

    print('\n### Original Sample Logs ###')
    print('Number of Entries:',chopper.size())
    print('Chopper Values:',chopper.value)
    print('Chopper Timestamps:',chopper.times)
    print('First Value:',chopper.firstValue())
    print('First Time:',chopper.firstTime())
    print('Second Value:',chopper.value[1])
    print('Second Time:',chopper.times[1])

    chopper.addValue('2010-Mar-26 00:00:00.00',5)
    print('\n--- Sample Logs with Addition ---')
    print('Number of Entries:',chopper.size())
    print('Chopper Values:',chopper.value)
    print('Chopper Timestamps:',chopper.times)

    # Also very nice is valueAsPrettyStr()
    print('\n# ', chopper.name)
    print(chopper.valueAsPrettyStr())

Output:

.. code-block:: python

    ### Original Sample Logs ###
    Number of Entries: 2
    Chopper Values: [ 4.  4.]
    Chopper Timestamps: ['2010-03-25T16:09:27.930000000' '2010-03-25T16:11:05.000000000']
    First Value: 4.0
    First Time: 2010-03-25T16:09:27.930000000
    Second Value: 4.0
    Second Time: 2010-03-25T16:11:05.000000000

    --- Sample Logs with Addition ---
    Number of Entries: 3
    Chopper Values: [ 4.  4.  5.]
    Chopper Timestamps: ['2010-03-25T16:09:27.930000000' '2010-03-25T16:11:05.000000000'
     '2010-03-26T00:00:00.000000000']

    #  ChopperStatus1
    2010-Mar-25 16:09:27.930000000  4
    2010-Mar-25 16:11:05  4
    2010-Mar-26 00:00:00  5


Auto-filtered Access
####################

Sample Logs can be automatically filtered. If so, they will have been filtered by `run_status` and also by `period_log`
if the data has been captured in multiple separate periods. While it is easy to view a plot of the un/filtered logs
with the `Filtered` tickbox, here's how to access such values in a script:

.. code-block:: python

    # Only access filtered properties
    print(chopper.filtered_value)
    print(chopper.filtered_times)

    # Access all values
    print(chopper.unfiltered().value)
    print(chopper.unfiltered().times)


Invalid Values
--------------

The same access to values filtered by ``run_status`` or ``period_log``, also applies to those filtered for bad or
invalid values. Whether entries are invalid values is marked in a hidden log called `LOGNAME_invalid_values`, which can
be found with ` model.get_hidden_logs()`.

.. code-block:: python

    # import mantid algorithms and SampleLogsModel
    from mantid.simpleapi import *
    from mantidqt.widgets.samplelogs.model import SampleLogsModel

    ws = Load('ENGINX00228061_log_alarm_data.nxs')
    model = SampleLogsModel(ws)

    invalid_logs = model.get_logs_with_invalid_data()
    # -1:Fully Invalid, 1:Partially Invalid
    print("Dictionary of Invalid Logs:",invalid_logs)

    for count,key in enumerate(invalid_logs.keys()):
        log_data = ws.getRun().getLogData(key)
        invalid_entries = ws.getRun().getLogData(key + '_invalid_values').value
        filtered_entries = log_data.filtered_value
        unfiltered_entries = log_data.unfiltered().value
        print("\nInvalid Log {}:".format(count+1), log_data.name )
        print("Unfiltered Values:",unfiltered_entries)
        print('Which entries are good:',invalid_entries)
        print("Filtered Values:",filtered_entries)

    log_data_CT1 = ws.getRun().getLogData('cryo_temp1')
    status = ws.getRun().getLogData('Status')

    print('\n# ', log_data_CT1.name)
    print(log_data_CT1.valueAsPrettyStr())
    print('# ', status.name)
    print(status.valueAsPrettyStr())

In the output below, you will notice that all entries for `cryo_temp2` are invalid and so there is no filter applied.
For `cryo_temp1`, the value ``5.0`` is filtered out for being marked invalid, but the value ``3.0`` is also filtered
out as it occurred before the status log was set to RUNNING. For more information, see: :ref:`07_invalid_sample_logs`.

Output:

.. code-block:: python

    Dictionary of Invalid Logs: {'cryo_temp1': 1, 'cryo_temp2': -1}
    Invalid Log 1: cryo_temp1
    Unfiltered Values: [ 3.  5.  7.]
    Which entries are good: [ True False  True]
    Filtered Values: [ 7.]

    Invalid Log 2: cryo_temp2
    Unfiltered Values: [ 3.  5.  7.]
    Which entries are good: [False False False]
    Filtered Values: [ 3.  5.  7.]

    #  cryo_temp1
    2015-Mar-17 12:55:12  3
    2015-Mar-17 12:55:17  5
    2015-Mar-17 12:55:32  7

    #  Status
    2015-Mar-17 12:55:11  SETUP
    2015-Mar-17 12:55:17  RUNNING


Plotting Sample Logs
####################

Mantid helps you plot Sample Logs from a workspace, without formally accessing them.

* Produce a plot from the "Show Sample Logs" interface, by right-clicking on the log entry or double-clicking on the
  plot. Next, look for plot toolbar button to `generate a script` for producing this plot for some inspiration!

* Check out the source code for the tiled plot in :ref:`06_sample_logs`, which plots using something similar to:

.. code-block:: python

    axes.plot(CNCS_7860_event, ExperimentInfo=0, Filtered=True, LogName='SampleTemp')
