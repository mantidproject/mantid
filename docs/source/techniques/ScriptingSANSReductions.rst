.. _ScriptingSANSReductions:

Scripting SANS Reductions
=========================

.. contents:: Table of Contents
  :local:

Introduction
------------

SANS data from ISIS can be analysed using :ref:`Python <introduction_to_python>` commands. Below is an example of a basic reduction on LOQ.

.. code-block :: python

    from ISISCommandInterface import *

    LOQ()
    MaskFile('MASK.094AA')

    AssignSample('54431.raw')
    TransmissionSample('54435.raw', '54433.raw')
    AssignCan('54432.raw')
    TransmissionCan('54434.raw', '54433.raw')

    WavRangeReduction(3, 9)

To configure the reduction the following commands are available once the ISISCommandInterface module has been imported, using the following command:

.. code-block :: python

    from ISISCommandInterface import *

These commands are given here in the order they are likely to be found in a script file although, except were stated, the order does not matter:

``LOQ()`` ``SANS2D()``
----------------------

This specifies which instrument the data was collected from. Either ``LOQ()`` or ``SANS2D()``.
The layout of the detector banks and detector efficiency files are stored here.

*This line must be included and before the* ``MaskFile()`` *line.*

``UserPath(path)``
------------------

Sets the directory in which Mantid should look for user settings file if a full path was not specified.

*This must be specified before the* ``MaskFile()`` *line to have any effect.*

``DataPath(path)``
------------------

This can be used to specify an extra directory in which Mantid will look for run files.
The directories that have been set in the :ref:`Manage User Directories <ManageUserDirectories>` dialog,
or equivalently the :ref:`Mantid.user.properties <Properties File>` file, are also checked.

``MaskFile(file_name)``
-----------------------

This settings file can be either a full path or a filename found in the ``UserPath()``.
The settings here are overridden by the commands listed below.

.. code-block :: python

    UserPath("C:/SANS/masks")
    MaskFile("MASK.09A") # or MaskFile("C:/SANS/masks/MASK.09A")

``AssignSample(sample_run, reload=True, period)``
-------------------------------------------------

Specifies the run to analyse using the format ``InstRun#.extension``, e.g. ``SANS2D7777.nxs``.
This is one of the few commands that executes :ref:`Mantid algorithms <Algorithm>` when called,
the effects of most commands are only felt after the reduction starts.
On calling this function the experimental run is :ref:`loaded <algm-Load>` and component positions are :ref:`changed <algm-MoveInstrumentComponent>`
(normally the detector bank and sample).
Currently only ``reload=true`` is supported.

``TransmissionSample(sample, direct, reload=True, period_t, period_d)``
-----------------------------------------------------------------------

Specifies the runs that will be used to calculate the transmission fraction for the sample run.
``sample`` contains transmission monitor counts data for the sample when the sample is present,
``direct`` contains similar counts data when the sample position is empty (direct beam).
The workspaces are loaded and the transmission :ref:`IDF <InstrumentDefinitionFile>`, if one exists for the instrument,
is loaded into the workspaces when this command is encountered. The transmission fraction is calculated later.
The ``period_t`` and ``period_d`` are used when there are multi-period files and specify the period to use for the sample and direct run respectively.

``AssignCan(can_run, reload=True, period)``
-------------------------------------------

The can is a scattering run made under the same conditions as the experimental run but only the sample container is in the sample position.
Hence allowing the effect of the container to be removed. The run is specified using ``instrumentrunnumber.extension``, e.g. ``SANS2D7777.nxs``.
On calling this function the run is loaded to a workspace and the detector banks and other components are moved as applicable.
Currently only ``reload=true`` is supported.

``TransmissionCan(can, direct, reload=True, period_t, period_d)``
-----------------------------------------------------------------

Specify the transmission and direct beam runs that will be used for the analysis of the can run.
The runs are loaded and with transmission :ref:`IDF <InstrumentDefinitionFile>`, if applicable, when Python encounters this command.

``SetMonitorSpectrum(specNum, interp=False)``
---------------------------------------------

Specifies the number of the TOF spectrum that will be used to for monitor normalisation.
This value will be used in the next reduction that is called (e.g. with :ref:`WavRangeReduction() <SANSScriptingWavRangeReduction>`).

``TransFit(mode, lambdamin, lambdamax)``
----------------------------------------

Sets the method and range over which to calculate a fit for the variation of transmission fraction with wavelength.
These arguments are passed to the algorithm :ref:`algm-CalculateTransmission`.
There is an extra fit mode ``Off`` which causes the unfitted workspace produced by :ref:`algm-CalculateTransmission`
to be used and ``lambdamin`` or ``lambdamax`` then have no effect.

``Detector(det_name)``
----------------------

Sets the detector bank to use for the reduction e.g. ``front-detector``.
The lowest angle detector is assumed if this line is not given.

``SetPhiLimit(phimin, phimax, use_mirror=True)``
------------------------------------------------

Call this function to restrict the analysis to sectors of the detector.
``Phimin`` and ``phimax`` define the limits of the sector where ``phi=0`` is the x-axis and ``phi=90`` is the y-axis.
Setting ``use_mirror`` to true causes the mirror sector to be included.

.. _SANSScriptingWavRangeReduction:

``WavRangeReduction(wav_start, wav_end, full_trans_wav, name_suffix)``
----------------------------------------------------------------------

Assuming the mask file contains the correct analysis details one can proceed to calculate :math:`I(Q)` using the
``WavRangeReduction()`` function, which can be executed with no arguments.
The return value of ``WavRangeReduction()`` is the name of the final reduced workspace.
This function calls many algorithms ending with a call to :ref:`algm-Q1D` or :ref:`algm-Qxy`.

- ``wav_start``: the first wavelength to be in the output data.
- ``wav_end``: the last wavelength in the output data.

``BatchReduce(filename, format, plotresults=False, saveAlgs={'SaveRKH':'txt'},verbose=False, centreit=False)``
--------------------------------------------------------------------------------------------------------------

This analyses a list of files to analyse from a file, it calls :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>`.
The function is available after the following import:

.. code-block :: python

    from SANSBatchMode import *

The first argument is the name of a CSV file, where each line specifies the data for a single reduction (:ref:`in this format <ISIS_SANS_Batch_File-ref>`).
The ``format`` argument is used to specify whether to load raw or nexus files and
``saveAlgs`` is a Python dictionary that contains the name of the save algorithm to use and the extension that it should have.
**If two algorithms in the dictionary use the same extension the first file will be overwritten!**


``AddRuns(runs, instrument ='sans2d', saveAsEvent=False, binning = "Monitors", isOverlay = False, time_shifts = None, defType='.nxs', rawTypes=('.raw', '.s*', 'add','.RAW'), lowMem=False)``
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

This file adds a list of run files. The ``runs`` variable holds a list of runs which are to be added.
The variable ``instrument`` specifies which instrument is currently being used.
The variable ``saveAsEvent`` allows the user to add multiple event files in a combined event file.
The ``isOverlay`` flag determines if the times of the events and sample logs should be placed on top of each other.
This is only applied if ``saveAsEvent`` was selected.
The ``time_shifts`` variable is a list of additional time shifts which will be applied if ``isOverlay`` is selected.
*Note that there has to be exactly one less time time shift than files to be added.*

.. categories:: Techniques
