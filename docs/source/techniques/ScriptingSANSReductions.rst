.. _ScriptingSANSReductions:

Scripting SANS Reductions
=========================

.. contents:: Table of Contents
  :local:

Introduction
------------

SANS data from ISIS can be analysed using :ref:`Python <introduction_to_python>` commands. Below is an example of a basic reduction on LOQ.

.. code-block :: python

    #import Command Interface from sans library
    from sans.command_interface.ISISCommandInterface import *

    """
    Test script outlining the use of the command line interface to reduce and save SANS data.
    Data is taken from the Mantid Training Data Course Set: https://www.mantidproject.org/installation/index#sample-data

    """

    LOQ()
    MaskFile('MaskFile.toml')

    AssignSample('LOQ74044.nxs')
    TransmissionSample('LOQ74024.nxs', 'LOQ74014.nxs')
    AssignCan('LOQ74019.nxs')
    TransmissionCan('LOQ74020.nxs', 'LOQ74014.nxs')

    #Performs reduction and saves data to path using SaveRKH and SaveNexus algorithms
    WavRangeReduction(2.2, 10, saveAlgs={'SaveRKH': 'txt','SaveNexus': 'nxs'})

The commands are available once the ISISCommandInterface module has been imported, using the following line:

.. code-block :: python

    from sans.command_interface.ISISCommandInterface import *

These commands are given below in the order in which they are likely to be found in a script file although,
except were stated, the order does not matter.

Reference Commands
------------------

LOQ / SANS2D
^^^^^^^^^^^^

.. code-block :: python

   LOQ() #For LOQ
   SANS2D() #For SANS2D

This specifies which instrument the data was collected from. Either ``LOQ()`` or ``SANS2D()``.
The layout of the detector banks and detector efficiency files are stored here.

*This line must be included, and before the* ``MaskFile()`` *line.*

UserPath
^^^^^^^^

.. code-block :: python

   UserPath(path)

Sets the directory in which Mantid should look for user settings file if a full path was not specified.

*This must be specified before the* ``MaskFile()`` *line to have any effect.*

DataPath
^^^^^^^^

.. code-block :: python

   DataPath(path)

This can be used to specify an extra directory in which Mantid will look for run files.
The directories that have been set in the :ref:`Manage User Directories <ManageUserDirectories>` dialog,
or equivalently the :ref:`Mantid.user.properties <Properties File>` file, are also checked.

MaskFile
^^^^^^^^

.. code-block :: python

    UserPath("C:/SANS/masks")
    MaskFile("ExampleMask.toml") # or MaskFile("C:/SANS/masks/ExampleMask.toml")

This settings file can be either a full path or a filename found in the ``UserPath()``.
The settings here are overridden by the commands listed below.



AssignSample
^^^^^^^^^^^^

.. code-block :: python

    AssignSample(sample_run, reload=True, period)

Specifies the run to analyse using the format ``InstRun#.extension``, e.g. ``SANS2D7777.nxs``.
This is one of the few commands that executes :ref:`Mantid algorithms <Algorithm>` when called,
the effects of most commands are only felt after the reduction starts.
On calling this function the experimental run is :ref:`loaded <algm-Load>` and component positions are :ref:`changed <algm-MoveInstrumentComponent>`
(normally the detector bank and sample).
Currently only ``reload=true`` is supported.

TransmissionSample
^^^^^^^^^^^^^^^^^

.. code-block :: python

    TransmissionSample(sample, direct, relaod=True, period_t, period_d)

Specifies the runs that will be used to calculate the transmission fraction for the sample run.
``sample`` contains transmission monitor counts data for the sample when the sample is present,
``direct`` contains similar counts data when the sample position is empty (direct beam).
The workspaces are loaded and the transmission :ref:`IDF <InstrumentDefinitionFile>`, if one exists for the instrument,
is loaded into the workspaces when this command is encountered. The transmission fraction is calculated later.
The ``period_t`` and ``period_d`` are used when there are multi-period files and specify the period to use for the sample and direct run respectively.

AssignCan
^^^^^^^^^

.. code-block :: python

    AssignCan(can_run, reload=True, period)

The can is a scattering run made under the same conditions as the experimental run but only the sample container is in the sample position.
Hence allowing the effect of the container to be removed. The run is specified using ``instrumentrunnumber.extension``, e.g. ``SANS2D7777.nxs``.
On calling this function the run is loaded to a workspace and the detector banks and other components are moved as applicable.
Currently only ``reload=true`` is supported.

TransmissionCan
^^^^^^^^^^^^^^^

.. code-block :: python

    TransmissionCan(can, direct, reload=True, period_t, period_d)

Specify the transmission and direct beam runs that will be used for the analysis of the can run.
The runs are loaded and with transmission :ref:`IDF <InstrumentDefinitionFile>`, if applicable, when Python encounters this command.

SetMonitorSpectrum
^^^^^^^^^^^^^^^^^^^

.. code-block :: python

    SetMonitorSpectrum(specNum, interp=False)

Specifies the number of the TOF spectrum that will be used to for monitor normalisation.
This value will be used in the next reduction that is called (e.g. with :ref:`WavRangeReduction() <SANSScriptingWavRangeReduction>`).

TransFit
^^^^^^^^

.. code-block :: python

    TransFit(mode, lambdamin, lambdamax)

Sets the method and range over which to calculate a fit for the variation of transmission fraction with wavelength.
These arguments are passed to the algorithm :ref:`algm-CalculateTransmission`.
There is an extra fit mode ``Off`` which causes the unfitted workspace produced by :ref:`algm-CalculateTransmission`
to be used and ``lambdamin`` or ``lambdamax`` then have no effect.

Detector
^^^^^^^^

.. code-block :: python

    Detector(det_name)

Sets the detector bank to use for the reduction e.g. ``front-detector``.
The lowest angle detector is assumed if this line is not given.

SetPhiLimit
^^^^^^^^^^^

.. code-block :: python

    SetPhiLimit(phimin, phimax, use_mirror=True)

Call this function to restrict the analysis to sectors of the detector.
``Phimin`` and ``phimax`` define the limits of the sector where ``phi=0`` is the x-axis and ``phi=90`` is the y-axis.
Setting ``use_mirror`` to true causes the mirror sector to be included.

.. _SANSScriptingWavRangeReduction:

WavRangeReduction
^^^^^^^^^^^^^^^^^

.. code-block :: python

    WavRangeReduction(wav_start=None, wav_end=None, full_trans_wav=None, name_suffix=None,
                      combineDet=None, saveAlgs=None, save_as_zero_error_free=False, output_name=None,
                      output_mode=OutputMode.PUBLISH_TO_ADS, use_reduction_mode_as_suffix=False)

Assuming the mask file contains the correct analysis details one can proceed to calculate :math:`I(Q)` using the
``WavRangeReduction()`` function, which can be executed with no arguments.
The return value of ``WavRangeReduction()`` is the name of the final reduced workspace.
This function calls many algorithms ending with a call to :ref:`algm-Q1D` or :ref:`algm-Qxy`.
Several optional parameters can control different aspects of the reduction
    - ``wav_start``: the first wavelength to be in the output data.
    - ``wav_end``: the last wavelength in the output data.
    - ``full_trans_wav``: Whether to use default's instrument wavelength range for transmission correction calculation, default is false.
    - ``name_suffix``: Appends the created output workspace with this suffix
    - ``combineDet``: combineDet can be one of the following

       - 'rear': runs one reduction for the 'rear' detector data
       - 'front': run one reduction for the 'front' detector data, and rescale+shift 'front' data
       - 'both': run both the above two reductions
       - 'merged': run the same reductions as 'both' and additionally create a merged data workspace
       - None: run one reduction for whatever detector has been set as the current detector before running this method. If front apply rescale+shift)
    - ``saveAlgs``: A dict of save algorithms containing the names of the algorithms as key and the extension as value(ex: ``saveAlgs={'SaveRKH':'txt'}``).
    - ``save_as_zero_error_free``: Should the reduced workspaces contain zero errors.
    - ``output_name``: Name of the output file. Default is sample run number.
    - ``output_mode``: Decides the output of the reduced data, whether to publish to the ads (``OutputMode.PUBLISH_TO_ADS``), save to file with the chosen algorithm
      in ``saveAlgs`` (``OutputMode.SAVE_TO_FILE``) or doing both(``OutputMode.BOTH``). ``OutputMode`` can be imported
      with ``from sans.common.enums import OutputMode``.
      If this parameter is omitted, the default behaviour will be to publish the output to the ads and save it in a file if there is a ``saveAlgs``.
    - ``use_reduction_mode_as_suffix``: If ``True``, appends second suffix to output name based on reduction mode.


BatchReduce
^^^^^^^^^^^

.. code-block :: python

    BatchReduce(filename, plotresults=False, saveAlgs=None,
                centreit=False, combineDet=None, save_as_zero_error_Free=False,
                output_mode=OutputMode.PUBLISH_TO_ADS)

This parses a list of files to analyse from a batch file, then it calls :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>` to perform each reduction.
The filename is a mandatory parameter:

    - ``filename``: Name of a CSV file included in the path, where each line specifies the data for a single reduction (:ref:`in this format <ISIS_SANS_Batch_File-ref>`).

Optional parameters:

    - ``plotresults``: If true, a graph with the results from each reduction will be created (only when it is called from Mantid).
    - ``saveAlgs``: Same as :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>`.
    - ``centreit``: Do centre finding (False by default).
    - ``combineDet``: Same as :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>`.
    - ``save_as_zero_error_free``: Same as :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>`.
    - ``output_mode``: Same as :ref:`WavRangeReduction <SANSScriptingWavRangeReduction>`..

Function returns a dictionary with some values from the reduction. (scale and shift as of now).


AddRuns
^^^^^^^

.. code-block :: python

    AddRuns(runs, instrument ='sans2d', saveAsEvent=False, binning = "Monitors",
            isOverlay = False, time_shifts = None, defType='.nxs',
            rawTypes=('.raw', '.s*', 'add','.RAW'), lowMem=False)


This file adds a list of run files. The ``runs`` variable holds a list of runs which are to be added.
The variable ``instrument`` specifies which instrument is currently being used.
The variable ``saveAsEvent`` allows the user to add multiple event files in a combined event file.
The ``isOverlay`` flag determines if the times of the events and sample logs should be placed on top of each other.
This is only applied if ``saveAsEvent`` was selected.
The ``time_shifts`` variable is a list of additional time shifts which will be applied if ``isOverlay`` is selected.
*Note that there has to be exactly one less time time shift than files to be added.*

.. categories:: Techniques
