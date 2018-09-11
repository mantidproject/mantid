.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Use by the Magnetism Reflectometer reduction. This algorithm takes in a data file and returns a workspace
group containing a workspace for each cross-section.

Users can provide log entry names for the logs defining the initial and final polarization states.
The output workspace group will contain a workspace for each combination of the initial and final
states with values 1 or 0 (on or off). Whether on and off correspond to spin up or down will
depend on the hardware used at the instrument, which can be determined from the logs.

The algorithm will read the "Analyzer" and "Polarizer" logs to determine whether those
devices were used. Filtering of the initial (final) state will only be done if the "Polarizer"
("Analyzer") log is greater than 0. A value of 0 means that the device was not in use.

Finally, a veto log can be provided for each of the initial and final state logs. Events with
a veto log other than 0 will be rejected.

For data files created using the legacy data acquisition system (pre-2018), the workspace group will
contain the four nexus entries of the original nexus file.

.. categories::

.. sourcelink::
