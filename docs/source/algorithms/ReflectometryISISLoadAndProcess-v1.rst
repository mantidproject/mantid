.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


ReflectometryISISLoadAndProcess
-------------------------------

This algorithm performs full preparation and processing for a single run or combined set of runs in an ISIS reflectometry reduction. Note that a set of runs here is a set that will be combined prior to reduction (it does not deal with a group of runs that will be post-processed after reduction). This algorithm is primarily aimed at reducing a single row in the table on the ``ISIS Reflectometry`` interface.

The steps this algorithm performs are:

- Ensure the required workspaces are loaded.
- Optionally sum multiple runs into a single input run prior to reduction.
- Optionally sum multiple transmission runs into a single input prior to reduction.
- Optionally perform time-slicing of the input run.
- Validate that the workspaces are the correct type.
- Run ReflectometryReductionOneAuto to perform the reduction.

Input runs and transmission runs are loaded if required, or existing workspaces are used if they are already in the ADS. When runs are loaded, they are named based on the run number with a ``TOF_`` or ``TRANS_`` prefix. To determine whether workspaces are already in the ADS, workspace names are matched based on the run number with or without this prefix. Other naming formats are not considered to match.

Input runs can be combined before reduction by supplying a comma-separated list of run numbers. They will be summed using the :ref:`algm-Plus` algorithm. Similarly, multiple input runs for the first and/or second transmission workspace inputs can be summed prior to reduction.

For time slicing, the input run must be an event workspace. If the workspace for the run already exists in the ADS but is the incorrect type, or does not have monitors loaded, it will be reloaded.

Input properties for the reduction are the same as those for :ref:`algm-ReflectometryReductionOneAuto`.

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

    Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460']

**Example: Sum multiple input runs**

.. testcode:: ExSumRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460, INTER13462')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumRuns

    Workspaces in the ADS after reduction: ['IvsQ_13460+13462', 'IvsQ_binned_13460+13462', 'TOF_13460', 'TOF_13460+13462', 'TOF_13462']

**Example: Sum multiple transmission runs into a single input**

.. testcode:: ExSumTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463,INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSumTransmissionRuns

   Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13463+13464', 'TRANS_13464']

**Example: Two separate transmission run inputs**

.. testcode:: ExCombineTransmissionRuns

    ReflectometryISISLoadAndProcess(InputRunList='INTER13460',FirstTransmissionRunList='INTER13463',
                                    SecondTransmissionRunList='INTER13464')
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExCombineTransmissionRuns

   Workspaces in the ADS after reduction: ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13464', 'TRANS_LAM_13463', 'TRANS_LAM_13464']

**Example: Slice input run**

.. testcode:: ExSliceRun

    ReflectometryISISLoadAndProcess(InputRunList='INTER38415', SliceWorkspace=True, TimeInterval=210)
    names = mtd.getObjectNames()
    names.sort()
    print('Workspaces in the ADS after reduction: {}'.format(names))

Output:

.. testoutput:: ExSliceRun

   Workspaces in the ADS after reduction: ['IvsLam_38415', 'IvsLam_38415_1', 'IvsLam_38415_2', 'IvsLam_38415_3', 'IvsQ_38415', 'IvsQ_38415_1', 'IvsQ_38415_2', 'IvsQ_38415_3', 'IvsQ_binned_38415', 'IvsQ_binned_38415_1', 'IvsQ_binned_38415_2', 'IvsQ_binned_38415_3', 'TOF_38415', 'TOF_38415_monitors', 'TOF_38415_sliced', 'TOF_38415_sliced_1', 'TOF_38415_sliced_2', 'TOF_38415_sliced_3']

.. seealso :: Algorithm :ref:`algm-ReflectometrySliceEventWorkspace`, :ref:`algm-ReflectometryReductionOneAuto` and the ``ISIS Reflectometry`` interface.

.. categories::

.. sourcelink::
