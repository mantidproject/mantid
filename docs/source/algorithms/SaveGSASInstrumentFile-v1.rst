.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Convert Fullprof's instrument resolution file (.irf) to GSAS's
instrument file (.iparm/.prm).

Supported peak profiles
#######################

-  Time-of-flight back-to-back exponential convoluted with pseudo-voigt
   (in future)

   -  Fullprof: Profile 9;
   -  GSAS: Type 3 TOF profile.

-  Thermal neutron time-of-flight back-to-back exponential convoluted
   with pseudo-voigt (implemented)

   -  Fullprof: Profile 10;
   -  GSAS: tabulated peak profile.

Supported input Fullprof file
#############################

There can be several types of Fullprof files as the input file

-  resolution file .irf (implemented)
-  configuration file .pcr (in future)

Set up :math:`2\theta`
######################

There are several places in this algorithm that can set the value of
:math:`2\theta`. From the highest priority, here is the list how
:math:`2\theta` is set up.

1. Algorithms' input property ``TwoTheta``;
2. Either input TableWorkspace or input Fullprof resolution (.irf) file;
3. Hard coded default  "TwoTheta" of a certain instrument.

Set up :math:`L_1`
##################

There are 2 places in this algorithm that can set the value of
:math:`L_1`. From the highest priority, here is the list how
:math:`2\theta` is set up.

1. Algorithms' input property ``L1``;
2. Hard coded default  "TwoTheta" of a certain instrument.

Calculation of :math:`L_2`
##########################

-  If "TwoTheta" (:math:`2\theta`) is given, L2 will be calculated from
   given 2Theta and L1 by
   :math:`DIFC = 252.816\cdot2sin(\theta)\sqrt{L1+L2}`. Notice that
   :math:`2\theta` given in input .irf file may have subtle difference
   to "2Theta", which is input by user in order to calculate L2.

-  If "2Theta" (:math:`2\theta`) is not given, L2 will be read from user
   input.

Usage
-----

**Example - save GSAS instrument file from a Fullprof .irf file:**

.. testcode:: ExHistSimple

  # Run the algorithm to save for GSAS instrument file
  SaveGSASInstrumentFile(InputFileName = "PG3_Bank1.irf", OutputFileName = "/tmp/PG3_Bank1.iparam",
    BankIDs = 1, Instrument = "powgen", ChopperFrequency = "60",
    IDLine = "60Hz 2011 Bank 1", Sample = "LaB6", L1 = 60.0, TwoTheta = 90.0)

  # Load GSAS parameter files
  gfile = open("/tmp/PG3_Bank1.iparam", "r")
  lines = gfile.readlines()
  gfile.close()

  # Print out some result
  print("Number of lines in GSAS instrument file:  {}".format(len(lines)))
  print("Line 0:  {}".format(lines[0].strip()))
  print("Line 1:  {}".format(lines[1].strip()))
  print("Line 2:  {}".format(lines[2].strip()))
  print("Line 3:  {}".format(lines[3].strip()))
  print("Line 305:  {}".format(lines[305].strip()))

.. testcleanup:: ExHistSimple

   import os
   os.remove("/tmp/PG3_Bank1.iparam")

Output:

.. testoutput:: ExHistSimple

  Number of lines in GSAS instrument file:  306
  Line 0:  12345678901234567890123456789012345678901234567890123456789012345678
  Line 1:  ID    60Hz 2011 Bank 1
  Line 2:  INS   BANK      1
  Line 3:  INS   FPATH1     60.000000
  Line 305:  INS  1PAB590   0.00213   0.46016   1.99061  -3.12296

.. categories::

.. sourcelink::
