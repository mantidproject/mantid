.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometryISISLoadAndProcess
-------------------------------

This algorithm performs full preparation and processing for a single run or combined set of runs in an ISIS reflectometry reduction. Note that a set of runs here is a set that will be combined prior to reduction (it does not deal with a group of runs that will be post-processed after reduction). This algorithm is primarily aimed at reducing a single row in the table on the ``ISIS Reflectometry`` interface.

The steps this algorithm performs are:

- Ensure workspaces are loaded and are of the correct type.
- Sum multiple input runs into a single workspace, if there is more than one.
- Sum multiple transmission runs into a single workspace, if there is more than one.
- Perform background subtraction, if requested.
- Perform time-slicing of the input run, if requested.
- Perform the reduction.
- Clean up the TOF workspaces into a group.

Input runs and transmission runs are loaded if required, or existing workspaces are used if they are already loaded. When runs are loaded, they are named based on the run number with a ``TOF_`` or ``TRANS_`` prefix. To determine whether workspaces are already loaded, workspace names are matched based on the run number with or without this prefix. Other naming formats are not considered to match.

If a value is provided for the ``CalibrationFile`` property then calibration will be applied for input runs that are loaded (but not where existing workspaces are being used). The calibration will be performed using :ref:`algm-ReflectometryISISCalibration`. See the documentation for the required calibration file format.

If time slicing is enabled, the input run must be an event workspace and have monitors loaded; otherwise, it must be a histogram workspace. If the workspace already exists but is the incorrect type or is missing monitors, it will be reloaded.

Input runs can be combined before reduction by supplying a comma-separated list of run numbers. They will be summed using the :ref:`algm-Plus` algorithm. Similarly, multiple input workspaces for the first and/or second transmission inputs can be summed prior to reduction.

If ``SubtractBackground`` is true, then background subtraction will be performed using the :ref:`algm-ReflectometryBackgroundSubtraction` algorithm. There are various options for specifying how the subtraction should be done.

If ``SliceWorkspace`` is true, the time slicing will be performed using the :ref:`algm-ReflectometrySliceEventWorkspace` algorithm. Various options are available for specifying the filter to use and slicing can be done by time or by log value.

The reduction is performed using :ref:`algm-ReflectometryReductionOneAuto`.

Finally, the TOF workspaces are cleaned up by grouping them into a workspace group named ``TOF``. If a group by this name already exists then they will be added to that group.

Usage
-------

.. include:: ../usagedata-note.txt

**Example: Run reduction on a single run**

.. testcode:: ExSingleRun

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSingleRun

    Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF', 'TOF_13460']

**Example: Sum multiple input runs**

.. testcode:: ExSumRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460, INTER13462')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumRuns

    Workspaces in the ADS after reduction: ['IvsQ_13460+13462', 'IvsQ_binned_13460+13462', 'TOF', 'TOF_13460', 'TOF_13460+13462', 'TOF_13462']

**Example: Sum multiple transmission runs into a single input**

.. testcode:: ExSumTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463,INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumTransmissionRuns

	Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF', 'TOF_13460', 'TRANS_13463', 'TRANS_13463+13464', 'TRANS_13464', 'TRANS_LAM_13463+13464']

**Example: Two separate transmission run inputs**

.. testcode:: ExCombineTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463',
                                    SecondTransmissionRunList='INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExCombineTransmissionRuns

   Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF', 'TOF_13460', 'TRANS_13463', 'TRANS_13464', 'TRANS_LAM_13463_13464']

**Example: Slice input run**

.. testcode:: ExSliceRun

    ReflectometryISISLoadAndProcess(InputRunList='INTER38415', SliceWorkspace=True, TimeInterval=210)
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSliceRun

   Workspaces in the ADS after reduction: ['IvsLam_38415', 'IvsLam_38415_sliced_0_210', 'IvsLam_38415_sliced_210_420', 'IvsLam_38415_sliced_420_610', 'IvsQ_38415', 'IvsQ_38415_sliced_0_210', 'IvsQ_38415_sliced_210_420', 'IvsQ_38415_sliced_420_610', 'IvsQ_binned_38415', 'IvsQ_binned_38415_sliced_0_210', 'IvsQ_binned_38415_sliced_210_420', 'IvsQ_binned_38415_sliced_420_610', 'TOF', 'TOF_38415', 'TOF_38415_monitors', 'TOF_38415_sliced', 'TOF_38415_sliced_0_210', 'TOF_38415_sliced_210_420', 'TOF_38415_sliced_420_610']

.. seealso :: Algorithm :ref:`algm-ReflectometrySliceEventWorkspace`, :ref:`algm-ReflectometryReductionOneAuto` and the ``ISIS Reflectometry`` interface.

.. categories::

.. sourcelink::
