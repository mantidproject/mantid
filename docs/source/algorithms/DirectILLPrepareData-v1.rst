.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm prepares data for the other algorithms in ILL's time-of-flight data reduction suite. Thus, it is usually the first algorithm to call in the reduction process. The algorithm performs some common basic data reduction steps to the raw data, and provides other workspaces, such as flat background information, which can be used in subsequent reduction steps. The workflow of the algorithm is shown in the diagram below:

.. diagram:: DirectILLPrepareData-v1_wkflw.dot

Input data
##########

Either *InputFile* or *InputWorkspace* has to be specified. *InputFile* can take multiple run numbers. In this case the files will be merged using the :ref:`MergeRuns <algm-MergeRuns>` algorihtm. For example, `'/data/0100:0103,0200:0202'` would merge runs 100, 101, 102, 103, 200, 201 and 202 from directory `/data/`.

Basic reduction steps
#####################

Some basic reduction steps are done to the input data.

#. Adjust the TOF axis so that the elastic time-of-flight corresponds to the L1+L2 distances (:ref:`CorrectTOFAxis <algm-CorrectTOFAxis>`).
#. Separate monitor and detector spectra to different workspaces (:ref:`ExtractMonitors <algm-ExtractMonitors>`).
#. Optionally normalise the detector specta to monitor counts or acquisition time (:ref:`NormaliseToMonitor <algm-NormaliseToMonitor>`).
#. Subtract time-independent background from the detector spectra (:ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`).
#. Optionally find the elastic peak positions for later use (:ref:`FindEPP <algm-FindEPP>`).
#. Optionally calibrate the incident energy (IN4 and IN6 spectrometers only, :ref:`GetEiMonDet <algm-GetEiMonDet>`).

More detailed description of some of these steps is given below.

TOF axis adjustment
^^^^^^^^^^^^^^^^^^^

The TOF axis is adjusted according to the elastic channel number found in the 'Detector.elasticpeak' sample log.

Normalisation to monitor
^^^^^^^^^^^^^^^^^^^^^^^^

If *Normalisation* is set to 'Normalisation Monitor', the monitor spectrum specified by the *Monitor* property is used for normalisation. A flat background is subtracted from the spectrum (no scaling applied), and it is integrated over the range specified by *ElasticPeakWidthInSigmas*. The monitor peak is found using :ref:`FindEPP <algm-FindEPP>`.


Flat background subtraction
^^^^^^^^^^^^^^^^^^^^^^^^^^^

A flat time-independent background for subtraction can be given by *FlatBkgWorkspace*. If this input property is not specified, flat background will be calculated from the detector spectra by (:ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`) using the 'Moving Average' mode. The *FlatBkgAveragingWindow* property is passed directly to (:ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`) as *AveragingWindowWidth*.

Before subtraction, the background workspace is multiplied by *FlatBkgScaling*.

The background used for the subtraction can be retrieved using the *OutputFlatBkgWorkspace* property. This property holds either the same workspace as *FlatBkgWorkspace*, or a workspace created by :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`. Note that no *FlatBkgScaling* is applied to this workspace. 

Incident energy calibration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Incident energy is calibrated either by giving a new energy as a single-value workspace in *IncidentEnergyWorkspace* or calculating it from the elastic peak positions. The elastic peak position can be given by *EPPWorkspace*. If this parameter not specified, :ref:`FindEPP <algm-FindEPP>` is used.

The calibrated energy can be retrieved as a single-value workspace using the *OutputIncidentEnergyWorkspace* property.

This step applies to IN4 and IN6 only.

Optional inputs and outputs
###########################

The algorithm has some optional input and output workspaces. Their purpose is to extract some common information from a single data set and use it as input for other algorithms or data sets. An example would be backgrounds extracted from a low temperature measurement which can be used when reducing data taken at higher temperatures.

The optional input and output workspaces come in pairs. If the input workspace is specified, it will be used in the reduction and returned as the corresponding output workspace. If the input workspace is not specified, the needed information is calculated from the current spectra, and returned in the output workspace.

* *EPPWorkspace* --- *OutputEPPWorkspace*: elastic peak position table, used for incident energy calibration, but also in :ref:`DirectILLDetectorDiagnostics <algm-DirectILLDetectorDiagnostics>` and :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>`.
* *IncidentEnergyWorkspace* --- *OutputIncidentEnergyWorkspace*: single-valued workspace containing calibrated incident energy, used for incident energy calibration.
* *FlatBkgWorkspace* --- *OutputFlatBkgWorkspace*: a MatrixWorkspace containing the flat backgrounds. Used for flat background subtraction and in :ref:`DirectILLDetectorDiagnostics <algm-DirectILLDetectorDiagnostics>`. Note that *FlatBkgScaling* is not applied to *OutputFlatBkgWorkspace*.


Usage
-----

**Example - Not implemented**

.. categories::

.. sourcelink::
