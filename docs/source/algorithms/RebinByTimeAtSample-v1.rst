
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm is used to determine the relative time (since the start of the run) that events interacted with the sample. Histogramming via rebinning allows statistics to be increased and
output data sizes to be made manageable.
This is useful for overlaying fast log data and filtered events workspaces. The filtering calculations are the same as used in
:ref:`algm-GenerateEventsFilter` for the elastic case. **Note that the current implementation of this algorithm assumes elastic scattering only.**

.. math::

   TofFactor = \frac{L1}{L1 + L2}

Where *TOfFactor* is used to account for flight paths.


This algorithm rebins an EventWorkspace according to both the pulse times of each event,
the time of flight, and the L1 and L2 distances. This algorithm is an extension
of :ref:`algm-Rebin` and :ref:`algm-RebinByPulseTimes`, which rebin by TOF an PulseTime
respectfully.

The Params inputs may
be expressed in an identical manner to the :ref:`algm-Rebin` algorithm. Param arguments are
in seconds since run start. Users may either provide a single value, which is interpreted as the
*step* (in seconds), or three comma separated values *start*, *step*,
*end*, where all units are in seconds, and start and end are relative to
the start of the run.

The x-axis is expressed in relative time to the start of the run in
seconds.

This algorithm may be used to diagnose problems with the electronics or
data collection. Typically, detectors should see a uniform distribution
of the events generated between the start and end of the run. This
algorithm allows anomalies to be detected.


Usage
-----
This algorithm takes the same inputs as :ref:`algm-RebinByPulseTimes`. See that algorithm for a usage guide.

.. categories::

.. sourcelink::
