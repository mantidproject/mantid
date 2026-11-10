.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometryISISLoadAndProcess
-------------------------------

This algorithm performs full preparation and processing for a single run or combined set of runs in an ISIS reflectometry reduction. Note that a set of runs here is a set that will be combined prior to reduction (it does not deal with a group of runs that will be post-processed after reduction). This algorithm is primarily aimed at reducing a single row in the table on the ``ISIS Reflectometry`` interface.

The steps this algorithm performs are:

- Ensure workspaces are loaded and are of the correct type.
- Create one working TOF copy for a single input, or use the merged result for multiple inputs.
- Sum multiple input runs into a single workspace, if there is more than one.
- Sum multiple transmission runs into a single workspace, if there is more than one.
- Apply detector calibration to each summed workspace, if requested.
- Perform background subtraction, if requested.
- Perform time-slicing of the input run, if requested.
- Perform the reduction (including polarization correction if requested).
- Clean up the TOF workspaces into a group.

Input runs and transmission runs are loaded if required, or existing workspaces are used if they are already loaded. When runs are loaded, they are named based on the run number with a ``TOF_`` or ``TRANS_`` prefix. To determine whether workspaces are already loaded, workspace names are matched based on the run number with or without this prefix. Other naming formats are not considered to match.

If a value is provided for the ``CalibrationFile`` property, input runs are summed before the resulting workspace is calibrated using :ref:`algm-ReflectometryISISCalibration`. The initial TOF cache is preserved. Every reduction operates on fresh working copies, so changing calibration or angle correction settings does not require reloading the datasets. Existing TOF workspaces are assumed to contain the initial data; calibration logs are not used to decide whether to reload them.

If a calibration file is provided for POLREF data, a specific POLREF calibration workflow is invoked via :ref:`algm-ReflectometryISISCalibration`. In this case, ``CorrectDetectors`` must be set to ``False`` as the detector position correction is applied as part of the calibration workflow. For this workflow, the experiment angle is taken from ``ThetaIn`` or, when ``ThetaIn`` is not supplied, from the run log selected by ``ThetaLogName``. At least one of ``NominalSpecularPixelSpectrumNo`` and ``ActualSpecularPixelSpectrumNo`` must be supplied (can be fractional):

- If both ``NominalSpecularPixelSpectrumNo`` and ``ActualSpecularPixelSpectrumNo`` are supplied, the experiment angle is corrected by the difference between their angular positions.
- If only ``NominalSpecularPixelSpectrumNo`` is supplied the actual specular pixel is obtained by fitting the specular peak on the summed sample workspace, then the experiment angle is corrected by the difference between actual and nominal angular positions.
- If only ``ActualSpecularPixelSpectrumNo`` is supplied the nominal pixel is set to equal this supplied value, making angle correction redundant.

The corrected experiment angle is used both for detector calibration and reduction. Transmission workspaces are aligned at zero experiment angle using the nominal pixel.

If angle correction is applied, a ``reflectometry_adjusted_theta`` log is added to the workspace. This log describes the processed working workspace; the initial cached workspace remains unchanged.

Working workspaces are passed directly between child algorithms. Calibration and
detector-position correction reuse these owned copies in place. For multiple runs,
``MergeRuns`` creates the working result directly from the cached originals, without
cloning each original first. Separate outputs needed for slicing, bank summation,
and unit conversion are still created.

Ordinary working copies remain outside the ADS. Temporary registrations are used
only by the slicing and workspace-group paths that require workspace names, and
are removed after processing, including on failure. Cached TOF data, geometry, and
event monitors remain available for subsequent reductions.

After a successful reduction, the full-detector processed sample is retained in
the ADS as ``<binned-output-name>_processed_tof`` (with a ``__`` prefix when inputs
are hidden). This is the existing working workspace, not another copy. For summed
runs it contains the combined data; for sliced reductions it retains the full-run
input. It includes calibration and any in-place detector correction or ROI masking
applied during processing. Bank summation does not replace this full-detector data.

When ``GroupTOFWorkspaces`` is enabled, the retained processed input and any slice
workspaces are collected into ``TOF_processed`` (or ``__TOF_processed`` when inputs
are hidden). Bank-summed workspaces previously collected in ``TOF``/``__TOF`` are
collected in the processed group instead, using the same end-of-workflow lookup. Grouped inputs and slices are flattened into TOF members in this
parent group. Redundant processed group containers are removed after their members
have been grouped; raw cache containers are retained. The original ``TOF``/``__TOF`` group contains cached inputs and their
monitors; bank-summed workspaces belong to the processed group. Flattening preserves
input and period-member order, and repeated grouping does not reorder existing members.

Reduced outputs and their full-detector diagnostic inputs carry a matching
``reflectometry_processed_tof_id`` log. Plotting finds the matching diagnostic
members in ``TOF_processed`` or ``__TOF_processed`` without retaining the original
processed group container. Detector-map and alignment plots use that association,
including the selected period for grouped inputs. The Runs table reads the adjusted
angle from the reduced output, independently of the diagnostic input's lifetime.
Detector-map and alignment plotting requires ``GroupTOFWorkspaces`` to be enabled.
When grouping is disabled, processed outputs and their group containers remain ungrouped.
Reduced outputs and bank-summed outputs retain their existing names.

If time slicing is enabled, the input run must be an event workspace and have monitors loaded; otherwise, it must be a histogram workspace. If the workspace already exists but is the incorrect type or is missing monitors, it will be reloaded.

Input runs can be combined before reduction by supplying a comma-separated list of run numbers. They will be summed using the :ref:`algm-MergeRuns` algorithm. Similarly, multiple input workspaces for the first and/or second transmission inputs can be summed prior to reduction.

If ``SubtractBackground`` is true, then background subtraction will be performed using the :ref:`algm-ReflectometryBackgroundSubtraction` algorithm. There are various options for specifying how the subtraction should be done.

If ``SliceWorkspace`` is true, the time slicing will be performed using the :ref:`algm-ReflectometrySliceEventWorkspace` algorithm. Various options are available for specifying the filter to use and slicing can be done by time or by log value.

The reduction is performed using :ref:`algm-ReflectometryReductionOneAuto`.

When the input is a workspace group, ``ROIDetectorIDs`` can be used if every member contains a compatible single
two-dimensional rectangular detector. Bank summing is applied independently to every group member while preserving
the group order, including when polarization analysis is enabled.

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

    Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'IvsQ_binned_13460_processed_tof', 'TOF', 'TOF_13460', 'TOF_processed']

**Example: Sum multiple input runs**

.. testcode:: ExSumRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460, INTER13462')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumRuns

    Workspaces in the ADS after reduction: ['IvsQ_13460+13462', 'IvsQ_binned_13460+13462', 'IvsQ_binned_13460+13462_processed_tof', 'TOF', 'TOF_13460', 'TOF_13462', 'TOF_processed']

**Example: Sum multiple transmission runs into a single input**

.. testcode:: ExSumTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463,INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumTransmissionRuns

	Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'IvsQ_binned_13460_processed_tof', 'TOF', 'TOF_13460', 'TOF_processed', 'TRANS_13463', 'TRANS_13464', 'TRANS_LAM_13463+13464']

**Example: Two separate transmission run inputs**

.. testcode:: ExCombineTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463',
                                    SecondTransmissionRunList='INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExCombineTransmissionRuns

   Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'IvsQ_binned_13460_processed_tof', 'TOF', 'TOF_13460', 'TOF_processed', 'TRANS_13463', 'TRANS_13464', 'TRANS_LAM_13463_13464']

**Example: Slice input run**

.. testcode:: ExSliceRun

    ReflectometryISISLoadAndProcess(InputRunList='INTER38415', SliceWorkspace=True, TimeInterval=210)
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSliceRun

   Workspaces in the ADS after reduction: ['IvsLam_38415', 'IvsLam_38415_sliced_0_210', 'IvsLam_38415_sliced_210_420', 'IvsLam_38415_sliced_420_610', 'IvsQ_38415', 'IvsQ_38415_sliced_0_210', 'IvsQ_38415_sliced_210_420', 'IvsQ_38415_sliced_420_610', 'IvsQ_binned_38415', 'IvsQ_binned_38415_processed_tof', 'IvsQ_binned_38415_sliced_0_210', 'IvsQ_binned_38415_sliced_210_420', 'IvsQ_binned_38415_sliced_420_610', 'TOF', 'TOF_38415', 'TOF_38415_monitors', 'TOF_38415_sliced_0_210', 'TOF_38415_sliced_210_420', 'TOF_38415_sliced_420_610', 'TOF_processed']

.. seealso :: Algorithm :ref:`algm-ReflectometrySliceEventWorkspace`, :ref:`algm-ReflectometryReductionOneAuto` and the ``ISIS Reflectometry`` interface.

.. categories::

.. sourcelink::
