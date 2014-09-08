.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes an ``InputWorkspace`` and ``Characterizations``
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`__ and 
creates a PropertyManager with the appropriate characterization runs. 
This is done by determining the effective accelerator frequency and 
center wavelength and choosing the appropriate row from the table.

This algorithm is one of the workflow algorithms that helps
:ref:`algm-SNSPowderReduction`.

Determing Frequency and Wavelength
##################################

The freqency is found by inspecting the logs (in order) "SpeedRequest1",
"Speed1", and "frequency". Whichever one has a nonzero value is used.
Simlilarly, the wavelength is taken by inspecting "Lambda". If either the 
frequency or wavelength cannot be determined, the algorithm will return a
default PropertyManager with mostly zeros.

PropertyManager Contents
########################

The PropertyManager will have the following keys and values. Listed in
the table is also their default values.

========== ======= =======
Name         Type  Default
========== ======= =======
frequency  double  0
wavelength double  0
bank       integer 1
vanadium   integer 0
container  integer 0
empty      integer 0
d_min      string  ""
d_max      string  ""
tof_min    double  0
tof_max    double  0
========== ======= =======

For a description of the  `TableWorkspace <TableWorkspace>`__ 
see :ref:`PDLoadCharacterizations <algm-PDLoadCharacterizations>`.

.. categories::
