.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is to load HFIR Spice powder diffraction data to MDWorkspaces.
HB2A is the only one instrument supported by this algorithm.
HB2A will be supported in future.

Inputs
######

Required workspaces
+++++++++++++++++++

There are two input Workspaces that are required for this algorithms.
Both of them stores the data from a SPICE file.

One is a TableWorkspace that stores the data, including detectors' counts
and sample environment logs' value, measured per data points.
The other is a MatrixWorkspace that stores the sample logs,
and serves the parent MatrixWorkspace for all temporary MatrixWorkspaces
that are created during the algorithm's execution.

These two workspaces can be obtained by executing algorithm LoasSpiceAscii.

Optional workspaces
+++++++++++++++++++

An optional TableWorkspace is for applying detectors' efficiency factor
to the raw detectors' counts.
It is required to a 2-column TableWorkspace.  Column 0 is of integer type for
detector IDs, while
Column 1 is of double type for detector efficiency factor (:math:`f`).
The corrected counts is equal to :math:`counts^{raw}/f`.

Outputs
#######

Two MDEventWorkspaces will be output from this algorithm.

One MDWorkspaces stores the experimental data.
Each MDEvent is a data point measured on one detector.
Thus if in the experiment, M detectors moves N times, then
there will be total :math:`M \times N` MDEvents.
It also stores the sample environment logs values in its ExperimentInfo.

The other MDWorkspaces stores the monitor counts of each detector
at each measurement.


Format of SPICE data file
#########################

There are two parts of content in SPICE data file.
One is the run information, in which line that starts with #.

In most cases, the run information is give as

# run info name = run info value

The other is the experimental data.
The first item is an integer as the index of experimental data point.

How to combine measurements
###########################

Powder diffractometers rotates its detectors to achieve the coverage
and resolution.
Hence the output of of a run should include all the measurements.

MDEventWorkspace is the best solution to combine all the measurements
to a single workspace and
keep all the information for future reduction.




Apply MDWorkspaces to HFIR powder diffractometer experiment
-----------------------------------------------------------

Run
###

In an HFIR powder diffractometer scan, one measurement is made at a certain rotational angle.
Such one measurement contains 44 detectors' counts, instrument setup (such as rotation angle) and
sample environment logs.
As it is translated to MDWorkspace, such a measurement constitutes a {\it run}.


Essential logs for building MDWorkspace
#######################################

There are a few sample logs that are essential to create the MDWorkspaces for the HFIR powder diffractometers.
The algorithm allows the user to specify these logs

* Rotation angle to determine the :math:`2\theta` for detector 0. The default is '2theta';
* Prefix of the logs for detectors.  The default is 'anode'.  In present HB2A's SPICE file, the detectors are labelled as 'anode1', 'anode2', and etc;
* Monitor counts for each measurement.  The default is 'monitor';
* Duration of each measurement.  The default is 'time'.


Sample Logs
###########

Sample logs will be written to the ExperimentInfo for each run.
There are N+1 ExperimentInfo in the MDWorkspace that stores
the detectors' counts.

The first N ExperimentInfo are for the N measurements.
The sample logs's value measured of that data point will be recorded in the corresponding
ExperimentInfo.
Hence each of these N ExperimentInfo will contains a set of sample logs, each of which
contains only one log entry.

The last one, i.e., ExperimentInfo[N], contains the combined sample logs from all the runs.
Hence for an experiment with N runs.


Temporary MD File
#################

In this algorithm, the MDEvnetWorkspaces are created by loading from a temporary MD file,
which is generated from a set of MatrixWorkspaces.
Each MatrixWorkspace stores the data for one individual measurement.

The format of the MD files are like ::

  DIMENSIONS
  x X m 100
  y Y m 100
  z Z m 100
  # Signal, Error, RunId, DetectorId, coord1, coord2, ... to end of coords
  MDEVENTS
  125 1 1 1 0.209057 0 1.98904
  133 1 1 2 0.30052 0 1.97729
  114 1 1 3 0.391584 0 1.96129
  130 1 1 4 0.485503 0 1.94018
  143 1 1 5 0.577963 0 1.91467
  135 1 1 6 0.667844 0 1.8852
  120 1 1 7 0.753968 0 1.85244
  115 1 1 8 0.840013 0 1.81504
  145 1 1 9 0.925819 0 1.77281
  117 1 1 10 1.00779 0 1.72753
  105 1 1 11 1.08951 0 1.67719
  102 1 1 12 1.16527 0 1.62547
  108 1 1 13 1.24041 0 1.56888
  110 1 1 14 1.31159 0 1.50988
  ... ...


If there are N detectors of the instruments and M measurements in total,
then there will be :math:`M\times N` MDEvents listed in the MD file.


Workflow
--------

The 2 input workspaces of algorithm ConvertSpiceDataToRealSpace are the output of
algorithm LoadSpiceAscii.
Therefore, in order to load an HB2A or HB2B data from a SPICE file,
LoadSpiceAscii should be called first.


Usage
-----

**Example - load a SPICE .dat file for HB2A:**

.. testcode:: ExLoadHB2ADataToMD

  # create table workspace and parent log workspace
  LoadSpiceAscii(Filename='HB2A_exp0231_scan0001.dat',
        IntegerSampleLogNames="Sum of Counts, scan, mode, experiment_number",
        FloatSampleLogNames="samplemosaic, preset_value, Full Width Half-Maximum, Center of Mass",
        DateAndTimeLog='date,MM/DD/YYYY,time,HH:MM:SS AM',
        OutputWorkspace='Exp0231DataTable',
        RunInfoWorkspace='Exp0231ParentWS')

  # load for HB2A
  ConvertSpiceDataToRealSpace(InputWorkspace='Exp0231DataTable',
        RunInfoWorkspace='Exp0231ParentWS',
        OutputWorkspace='Exp0231DataMD',
        OutputMonitorWorkspace='Exp0231MonitorMD')

  # output
  datamdws = mtd["Exp0231DataMD"]
  print("Number of events = {}".format(datamdws.getNEvents()))

.. testcleanup:: ExLoadHB2ADataToMD

  DeleteWorkspace('Exp0231DataTable')
  DeleteWorkspace('Exp0231ParentWS')
  DeleteWorkspace('Exp0231DataMD')
  DeleteWorkspace('Exp0231MonitorMD')

Output:

.. testoutput:: ExLoadHB2ADataToMD

  Number of events = 2684

.. categories::

.. sourcelink::
