.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads information into a
:ref:`TableWorkspace <Table Workspaces>` for the
characterization information and a collection of output parameters for
the focus positions to be used in :ref:`algm-EditInstrumentGeometry`. If
a section is missing then those parameters will be empty. This includes an empty
table (zero rows) if that information is missing. The resulting TableWorkspace
is intended to be used by :ref:`algm-PDDetermineCharacterizations`

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

The first line ``Instrument parameter file:`` must be present to mark
the beginning of the first section. Whatever string appears after the
semicolon is copied into the ``IParmFilename`` output property. The
following lines are of the form "bank l2 polar" with the last line
being the keyword ``L1`` followed by the effective primary flight
path. This information is saved in the ``IParmFilename``,
``SpectrumIDs``, ``L2``, ``Polar``, ``Azimuthal`` and
``PrimaryFlightPath`` properties. The ``Azimuthal`` values are
optional and will be set to zero if not specified. No example for
specifying azimuthal angles for the focus positions is being supplied
as it uncommon except for preferred orientation studies.

The second section of the characterizations file is read into the output
:ref:`TableWorkspace <Table Workspaces>` as described below.

A second example from NOMAD demonstrates how to specify different
ranges for each focused spectrum as well as the optional wavelength
ranges::

  Instrument parameter file: NOMAD_11_22_11.prm
  1 2 15
  2 2 31
  3 2 67
  4 2 122
  5 2 154
  6 2 7
  L1 19.5
  #S 1 characterization runs
  #L frequency(Hz) center_wavelength(angstrom) bank_num vanadium_run empty_run vanadium_back d_min(angstrom) d_max(angstrom) wl_min wl_max
  60 1.4  1 0 0 0 .31,.25,.13,.13,.13,.42 13.66,5.83,3.93,2.09,1.57,31.42 300.00 16666.67 .1 2.9

The :literal:`exp.ini` file is specific to the NOMAD instrument at SNS
and is optional. It will update the vanadium, container, and empty
instrument value in all rows of the table. This file is generally
discouraged. An example version of this file is::

  required *******************
  Dia 49262
  DiaBg 49257
  Vana 49258
  VanaBg 49086
  MTc 49257
  optional ******************
  recali yes
  renorm yes
  autotemp yes
  scan1 49464
  scanl 80000
  Hz    60
  # IPTS 14821

After realizing that the much of the information in the
characterizations file is independent of sample environment, a second
characterization file was designed to add to the information of the
first. The first line is to indicate the format of the file, and the
rest is whitespace delimited. There are 6 required columns, everything
past that is a :literal:`SampleContainer` identifier which will be
used to override the value that is in the original characterization
file. The :literal:`frequency` and :literal:`wavelength` columns are
still used as keys to determine which row contains the run
identifiers::

  version=1
  freq wl     van   van_back mt_env mt_instr PAC06 PAC08 PAC10
  60 0.533   27056   27050     0      0      27044 27032 27038
  60 1.066   27057   27051     0      0      27045 27033 27039
  60 1.333   27058   27052     0      0      27046 27034 27040
  60 2.665   27059   27053     0      0      27047 27035 27041
  60 3.731   27060   27054     0      0      27048 27036 27042
  60 4.797   27061   27055     0      0      27049 27037 27043
  10 3.198   27062       0     0      0          0     0     0

Characterization TableWorkspace
###############################

The columns names and types are described in the following table. Any
missing values are replaced with a zero which will generally skip that
bit of information.

============== =======
Name           Type
============== =======
frequency      double
wavelength     double
bank           int
vanadium       str
container      str
empty          str
d_min          str
d_max          str
tof_min        double
tof_max        double
wavelength_min double
wavelength_max double
============== =======

There can be any number of additional columns with the
:literal:`SampleContainer` (with spaces removed) for the column name,
and type of string.

Usage
-----

While there are many options for how to use this algorithm, the
suggestion is to supply the classic and version 1 characterizations in
a comma separated list as the :literal:`Filename` property.

.. code-block:: python

   filenames = ','.join(['PG3_char_2016_08_01-HR.txt','PG3_char_2016_02_15-PAC-single.txt'])
   PDLoadCharacterizations(Filename=filenames, OutputWorkspace='char')

.. categories::

.. sourcelink::
