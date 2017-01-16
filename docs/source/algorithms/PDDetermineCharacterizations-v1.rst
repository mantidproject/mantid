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
:ref:`algm-SNSPowderReduction` and its child algorithm
:ref:`algm-AlignAndFocusPowder`.

Determing Frequency and Wavelength
##################################

The freqency is found by inspecting the logs (in order)
``SpeedRequest1``, ``Speed1``, and ``frequency``. Whichever one has a
nonzero value is used.  Simlilarly, the wavelength is taken by
inspecting ``LambdaRequest`` then ``Lambda``. If either the frequency
or wavelength cannot be determined, the algorithm will return a
default PropertyManager with mostly zeros.

PropertyManager Contents
########################

The PropertyManager will have the following keys and values. Listed in
the table is also their default values. The "Alg Property" column is
what property of this algorithm is used to override what is in the
table. For all of those properties, "0" is interpreted as use the
information from the supplied table, "-1" is interpreted as set the
value to zero in the resulting ``PropertyManager``.

=================== ============ ======= ============
Name                Type         Default Alg Property
=================== ============ ======= ============
frequency           double       0
wavelength          double       0
bank                integer      1
vanadium            integerarray [0]     NormRun
vanadium_background integerarray [0]     NormBackRun
container           integerarray [0]     BackRun
empty_environment   integerarray [0]     EmptyEnv
empty_instrument    integerarray [0]     EmptyInstr
d_min               string       ""
d_max               string       ""
tof_min             double       0
tof_max             double       0
wavelength_min      double       0
wavelength_max      double       0
=================== ============ ======= ============

In the case of extra columns existing in the `TableWorkspace
<TableWorkspace>`__ denoting ``SampleContainer`` information: if the
``SampleContainer`` isn't a property on the workspace, or the value
isn't one of the column labels, the value of the ``container`` column
in the supplied `TableWorkspace <TableWorkspace>`__ will be used
instead.

For a description of the  `TableWorkspace <TableWorkspace>`__
see :ref:`PDLoadCharacterizations <algm-PDLoadCharacterizations>`.

.. categories::

.. sourcelink::
