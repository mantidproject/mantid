.. _06_sample_logs:

===========
Sample Logs
===========

Basic Inspection
----------------
When experiments are run, experimental information is saved as **Sample Logs**. They are simply a record of
certain quantities over time, e.g. SampleTemp.

To access these:

- Load data with logs attached (e.g. ``CNCS_7860_event.nxs``)
- Right-click on the workspace in the Workspace Toolbox and select "Show Sample Logs".

.. figure:: /images/SampleLogs.png
   :alt: Sample Logs for CNCS_7860_event
   :align: center

- Move through the logs with arrow keys or by scrolling and clicking on the log of interest. The plot will update
  for relevant logs.
- Use the *Relative Time* tickbox to change time between absolute (experiment timestamp)
  and relative (normal time axis).
- Right-clicking on a log (e.g. SampleTemp) you can produce a normal workbench plot of the selected log, or print its
  values to the Messages Box:

.. code-block:: none

    # SampleTemp
    2010-Mar-25 16:09:27.620000000  279.904
    2010-Mar-25 16:09:31.511000032  279.907
    2010-Mar-25 16:09:41.510999794  279.909
    2010-Mar-25 16:09:51.526000137  279.913
    2010-Mar-25 16:10:01.542000885  279.917
    2010-Mar-25 16:10:11.542000885  279.921
    2010-Mar-25 16:10:21.542000885  279.926
    2010-Mar-25 16:10:31.557999725  279.932
    2010-Mar-25 16:10:41.558003540  279.938
    2010-Mar-25 16:10:51.558003540  279.943
    2010-Mar-25 16:11:01.558003540  279.95
    2010-Mar-25 16:11:11.558003540  279.957
    2010-Mar-25 16:11:21.558003540  279.965
    2010-Mar-25 16:11:31.558003540  279.972
    2010-Mar-25 16:11:41.558003540  279.98
    2010-Mar-25 16:11:51.558003540  279.987

For more info, check out how to access :ref:`Sample logs with Python<04_run_logs>`.


Filtering
---------

As sample logs are recorded with timestamps, event data (where each neutron timestamp is recorded) can be filtered
into time intervals based on these sample logs.

As well as the info below, you may find our :ref:`Overview of Event Filtering<EventFiltering>` helpful.

Algorithms
##########

This can be performed directly with :ref:`algm-FilterByTime` or indirectly with :ref:`algm-FilterByLogValue`:

- Using the input parameters specified below, the CNCS_7860_event workspace can be
  filtered by the log value - ``SampleTemp``.

.. figure:: /images/FilterByLogValueTemp.png
   :alt: FilterByLogValue High
   :align: center

- Look at the effect this filter has on the SampleTemp log and in particular on the data in
  spectrum number 1000.
  Note that the time for the high temperature sample log data has reset its minimum value to 0 as it doesn't have the start
  time value needed to calculate the relative time (the start of the logs here is where we made the cut).

.. plot::
    :align: center

	# import mantid algorithms, matplotlib and plotSpectrum
	from mantid.simpleapi import *
	import matplotlib.pyplot as plt

	CNCS_7860_event = Load('CNCS_7860_event')

	CNCS_7860_event_high = FilterByLogValue(InputWorkspace='CNCS_7860_event', LogName='SampleTemp', MinimumValue=279.94, LogBoundary='Left')
	CNCS_7860_event_low = FilterByLogValue(InputWorkspace='CNCS_7860_event', LogName='SampleTemp', MaximumValue=279.94, LogBoundary='Left')

	fig, axes = plt.subplots(edgecolor='#ffffff', ncols=2, nrows=3, num='CNCS_7860_even: FilterByLogValue - SampleTemp 279.94', figsize = (7,10), subplot_kw={'projection': 'mantid'})

	'''Plot Data'''

	axes[0][1].plot(CNCS_7860_event, color='green', label='Full Data: spec 1000', specNum=1000)
	axes[0][1].set_xlabel('Time-of-flight ($\\mu s$)')
	axes[0][1].set_ylabel('Counts ($\\mu s$)$^{-1}$')
	axes[0][1].set_title('Full Temp Range: spec 1000')

	axes[1][1].plot(CNCS_7860_event_high, color='green', label='High Temp Data: spec 1000', specNum=1000)
	axes[1][1].set_xlabel('Time-of-flight ($\\mu s$)')
	axes[1][1].set_ylabel('Counts ($\\mu s$)$^{-1}$')
	axes[1][1].set_title('High Temp: spec 1000')

	axes[2][1].plot(CNCS_7860_event_low, color='green', label='Low Temp Data: spec 1000', specNum=1000)
	axes[2][1].set_xlabel('Time-of-flight ($\\mu s$)')
	axes[2][1].set_ylabel('Counts ($\\mu s$)$^{-1}$')
	axes[2][1].set_title('Low Temp: spec 1000')

	'''Plot Temperature Sample Log'''

	axes[0][0].axhline(279.94, color='red')
	axes[0][0].plot(CNCS_7860_event, ExperimentInfo=0, Filtered=True, LogName='SampleTemp', color='#1f77b4', drawstyle='steps-post', label='SampleTemp (K)')
	axes[0][0].set_xlabel('Time (s)')
	axes[0][0].set_ylabel('SampleTemp (K)')
	axes[0][0].set_title('Sample Log Temperature: All')
	temp_x_limit = axes[0][0].get_xlim()
	temp_y_limit = axes[0][0].get_ylim()

	axes[1][0].axhline(279.94, color='red')
	axes[1][0].plot(CNCS_7860_event_high, ExperimentInfo=0, Filtered=True, LogName='SampleTemp', color='#1f77b4', drawstyle='steps-post', label='SampleTemp (K)')
	axes[1][0].set_xlabel('Time (s)')
	axes[1][0].set_ylabel('SampleTemp (K)')
	axes[1][0].set_title('Sample Log Temperature: High')
	axes[1][0].set_ylim(temp_y_limit)
	axes[1][0].set_xlim(temp_x_limit[0]-145,temp_x_limit[1]-145)

	axes[2][0].axhline(279.94, color='red')
	axes[2][0].plot(CNCS_7860_event_low, ExperimentInfo=0, Filtered=True, LogName='SampleTemp', color='#1f77b4', drawstyle='steps-post', label='SampleTemp (K)')
	axes[2][0].set_xlabel('Time (s)')
	axes[2][0].set_ylabel('SampleTemp (K)')
	axes[2][0].set_title('Sample Log Temperature: Low')
	axes[2][0].set_xlim(temp_x_limit)
	axes[2][0].set_ylim(temp_y_limit)

	fig.tight_layout()
	fig.show()


Interface
#########

There is also an interface for FilteringEvents. From the top menu bar select ``Interface > Utility > FilterEvents``.
This is a more interactive way of filtering data. A few differences from the "Show Sample Logs" interface are that logs
are not plotted with a step drawstyle and a normal workbench plot cannot be created.

In the same way as above you can select the sample log of interest and set a Min/Max value. When you click 'Filter'
the following workspaces will be outputted to the Workspace Toolbox:
a TableWorkspace, which can be used as a SplitterWorkspace (an outline of where to apply the filter), and a
WorkspaceGroup containing the filtered and unfiltered data.

For more information on the other functionalities of this interface see: :ref:`Filter_Events_Interface`.

.. figure:: /images/FilterEventsInterface.png
   :alt: FilterEvents Interface
   :align: center
