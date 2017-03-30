.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm preprocesses data for the other algorithms in ILL's time-of-flight data reduction suite. Thus, it is usually the first algorithm to call in the reduction process. The algorithm (optionally) loads data from disk, performs some common basic data reduction steps to the raw data, and provides other workspaces, such as flat background information, which can be used in subsequent reduction steps. The workflow of the algorithm is shown in the diagram below:

.. diagram:: DirectILLCollectData-v1_wkflw.dot

Input data
##########

Either *InputFile* or *InputWorkspace* has to be specified. *InputFile* can take multiple run numbers. In this case the files will be merged using the :ref:`MergeRuns <algm-MergeRuns>` algorihtm. For example, `'/data/0100:0103,0200:0202'` would merge runs 100, 101, 102, 103, 200, 201 and 202 from directory `/data/`.

Basic reduction steps
#####################

Some basic reduction steps are done to the input data.

#. Separate monitor and detector spectra to different workspaces.
#. Optionally normalise the detector specta to monitor counts or acquisition time.
#. Subtract time-independent background from the detector spectra.
#. Optionally find the elastic peak positions.
#. Optionally calibrate the incident energy.
#. Adjust the TOF axis so that the elastic time-of-flight corresponds to the L1+L2 distances.
#. Find elastic peak positions again, if *OutputEPPWorkspace* is requested.

More detailed description of some of these steps is given below.

.. note::
    The initial time-of-flight axis of ILL's spectrometers has an arbitrary starting point. Therefore, the TOF values in the intermediate workspaces do not correspond to any physical flight distances until they are corrected at step 6. 

Normalisation to monitor
^^^^^^^^^^^^^^^^^^^^^^^^

If *Normalisation* is set to 'Normalisation Monitor', the monitor spectrum specified by the *Monitor* property is used for normalisation. A flat background is subtracted from the spectrum (no scaling applied), and it is integrated over the range specified by *ElasticPeakWidthInSigmas*. The monitor peak is found using :ref:`FindEPP <algm-FindEPP>`. If :ref:`FindEPP <algm-FindEPP>` fails to find a peak in the monitor spectrum, the entire monitor range is integrated.

Flat background subtraction
^^^^^^^^^^^^^^^^^^^^^^^^^^^

A flat time-independent background for subtraction can be given by *FlatBkgWorkspace*. If this input property is not specified, flat background will be calculated from the detector spectra by (:ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`) using the 'Moving Average' mode. The *FlatBkgAveragingWindow* property is passed directly to (:ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`) as *AveragingWindowWidth*.

Before subtraction, the background workspace is multiplied by *FlatBkgScaling*.

The background used for the subtraction can be retrieved using the *OutputFlatBkgWorkspace* property. This property holds either the same workspace as *FlatBkgWorkspace*, or a workspace created by :ref:`CalculateFlatBackground <algm-CalculateFlatBackground>`. Note that no *FlatBkgScaling* is applied to this workspace. 

Elastic peak positions (EPP)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Information on the elastic peaks (position, peak width) is needed for incident energy calibration, as well as for the :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>` and :ref:`DirectILLIntegrateVanadium <algm-DirectILLIntegrateVanadium>` algorithms. This data comes in the form of a EPP workspace which is a TableWorkspace containing columns specified by the :ref:`FindEPP <algm-FindEPP>` algorithm.

If no external EPP table is given by the *EPPWorkspace* property, the algorithm either fits the elastic peaks using :ref:`FindEPP <algm-FindEPP>`, or calculates their nominal positions using :ref:`CreateEPP <algm-CreateEPP>`. This behavior can be controlled by the *EPPCreationMode* property. In the calculation case, a nominal peak width can be given using the *Sigma* property.  The peak width is needed for some integration operations. If *Sigma* is not specified, ten times the first bin width in the workspace will be used.

Incident energy calibration
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Incident energy is calibrated either by giving a new energy as a single-value workspace in *IncidentEnergyWorkspace* or calculating it from the elastic peak positions. The elastic peak position can be given by *EPPWorkspace*. If this parameter not specified, :ref:`FindEPP <algm-FindEPP>` is used.

The calibrated energy can be retrieved as a single-value workspace using the *OutputIncidentEnergyWorkspace* property.

This step applies to IN4 and IN6 only.

TOF axis adjustment
^^^^^^^^^^^^^^^^^^^

The TOF axis is adjusted according to the elastic channel number found in the 'Detector.elasticpeak' sample log.

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
