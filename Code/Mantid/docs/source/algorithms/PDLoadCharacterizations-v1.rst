.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads information into a
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`__ for the 
characterization information and a collection of output parameters for 
the focus positions to be used in :ref:`algm-EditInstrumentGeometry`. If 
a section is missing then those parameters will be empty. This includes an empty
table (zero rows) if that information is missing. The resulting TableWorkspace
is intented to be used by :ref:`algm-PDDetermineCharacterizations`

This algorithm is one of the workflow algorithms that helps
:ref:`algm-SNSPowderReduction`.

Characterization File
#####################

An example from POWGEN shows both of the available portions of a 
characterization file::

  Instrument parameter file:
  1 3.18 90.0000
  L1 60.0
  #S 1 characterization runs
  #L frequency(Hz) center_wavelength(angstrom) bank_num vanadium_run empty_run vanadium_back d_min(angstrom) d_max(angstrom)
  60 0.533  1 17533 0 0 0.05  2.20  0000.00  16666.67
  60 1.066  2 17534 0 0 0.30  4.60  8333.33  25000.00
  60 1.333  3 17535 19770 0 0.43  5.40 12500.00  29166.67
  60 2.665  4 17536 0 0 1.15  9.20 33333.33  50000.00
  60 4.797  5 17537 0 0 2.25 15.35 66666.67  83333.67
  60 3.731  6 18280 0 0 1.70 12.50 50000.00  66666.67
  10 3.198  1 17538 0 0 0.05 15.40 00000.00 100000.00

The first line ``Instrument parameter file:`` must be present to mark the
beginning of the first section. Whatever string appears after the semicolon
is copied into the ``IParmFilename`` output property. The following lines 
are of the form "bank l2 polar" with the last line being the keyword ``L1``
followed by the effective primary flight path. This information is saved 
in the ``IParmFilename``, ``SpectrumIDs``, ``L2``, ``Polar``, and 
``PrimaryFlightPath`` properties. The ``Azimuthal`` properties is filled with zeros
and is the same length as ``SpectrumIDs``, ``L2``, ``Polar``, and ``PrimaryFlightPath``.

The second section of the characterizations file is read into the output
`TableWorkspace <http://www.mantidproject.org/TableWorkspace>`__ as described below.

Characterization TableWorkspace
###############################
The columns names and types are described in the following table.

========== =======
Name       Type
========== =======
frequency  double
wavelength double
bank       int
vanadium   int
container  int
empty      int
d_min      str
d_max      str
tof_min    double
tof_max    double
========== =======

.. categories::
