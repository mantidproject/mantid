.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Convert Fullprof's instrument resolution file (.irf) to GSAS's
instrument file (.iparm/.prm).

Supported peak profiles
#######################

-  Time-of-flight back-to-back exponential convoluted with pseudo-voigt
   (planned)

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
-  configuration file .pcr (planned)

Set up :math:`2\theta`
######################

There are several places in this algorithm that can set the value of
:math:`2\theta`. From the highest priority, here is the list how
:math:`2\theta` is set up.

| ``1. Algorithms' input property ``\ *``2Theta``*\ ``;``
| ``2. Either input TableWorkspace or input Fullprof resolution (.irf) file;``
| ``3. Hard coded default  ``\ :math:`2\theta`\ `` of a certain instrument.``

Set up :math:`L_1`
##################

There are 2 places in this algorithm that can set the value of
:math:`L_1`. From the highest priority, here is the list how
:math:`2\theta` is set up.

| ``1. Algorithms' input property ``\ *``L1``*\ ``;``
| ``2. Hard coded default  ``\ :math:`2\theta`\ `` of a certain instrument.``

Calculation of L2
#################

-  If 2Theta (:math:`2\theta`) is given, L2 will be calculated from
   given 2Theta and L1 by
   :math:`DIFC = 252.816\cdot2sin(\theta)\sqrt{L1+L2}`. Notice that
   :math:`2\theta` given in input .irf file may have subtle difference
   to "2Theta", which is input by user in order to calculate L2.

-  If "2Theta" (:math:`2\theta`) is not given, L2 will be read from user
   input.

.. categories::
