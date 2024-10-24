.. _CreateIkedaCarpenterParametersGSAS:

Create Ikeda Carpenter Parameters GSAS
======================================

.. contents:: Table of Contents
  :local:

Introduction
------------

Here we show how to take a Fullprof parameter file containing :ref:`Ikeda-Carpenter-Pseudo-Voigt <func-IkedaCarpenterPV>` parameters and translate these
into equivalent parameters for the Mantid implementation of this function.

Annotating GSAS GEM instrument parameter file
---------------------------------------------

This GSAS file is one generated for the GEM instrument at ISIS.
The syntax is described in some detail from page 221 in the GSAS Technical Manual in section entitled "Instrument Parameter File".

.. code-block :: text

    COMM  Y2O3  +  Si  cycle 13/2
    INS   BANK  6
    INS   HTYPE   PNTR
    INS  1 ICONS    746.96     -0.24      3.04
    INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1
    INS  1I ITYP    0    1.000     25.000       2
    INS  1PRCF      2   15   0.00100
    INS  1PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  1PRCF 2   0.000000E+00   0.176802E+03   0.000000E+00   0.000000E+00
    INS  1PRCF 3   0.000000E+00   0.000000E+00   0.000000E+00   0.000000E+00
    INS  1PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00
    INS  2 ICONS   1482.98      0.98     12.65
    INS  2BNKPAR    1.7714     17.98      0.00    .00000     .3000    1    1
    INS  2I ITYP    0    1.000     21.000       2
    INS  2PRCF      2   15   0.00100
    INS  2PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  2PRCF 2   0.000000E+00   0.295572E+03  -0.134662E+01   0.000000E+00
    INS  2PRCF 3   0.361229E+01   0.000000E+00   0.000000E+00   0.000000E+00
    INS  2PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00
    INS  3 ICONS   2813.67      1.41      6.22
    INS  3BNKPAR    1.4450     34.96      0.00    .00000     .3000    1    1
    INS  3I ITYP    0    1.000     20.000       2
    INS  3PRCF      2   15   0.00100
    INS  3PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  3PRCF 2   0.000000E+00   0.280289E+03   0.272947E+01   0.000000E+00
    INS  3PRCF 3   0.444792E+01   0.000000E+00   0.000000E+00   0.000000E+00
    INS  3PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00
    INS  4 ICONS   4844.99     -1.73     -1.50
    INS  4BNKPAR    1.2212     63.62      0.00    .00000     .3000    1    1
    INS  4I ITYP    0    1.000     20.000       2
    INS  4PRCF      2   15   0.00100
    INS  4PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  4PRCF 2   0.000000E+00   0.202940E+03   0.446899E+01   0.000000E+00
    INS  4PRCF 3   0.416948E+01   0.000000E+00   0.000000E+00   0.000000E+00
    INS  4PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00
    INS  5 ICONS   6662.11     -4.58     -2.30
    INS  5BNKPAR    1.3793      91.37     0.00    .00000     .3000    1    1
    INS  5I ITYP    0    1.000     18.000       2
    INS  5PRCF      2   15   0.00100
    INS  5PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  5PRCF 2   0.000000E+00   0.111084E+03  -0.175113E+01   0.000000E+00
    INS  5PRCF 3   0.284103E+01   0.000000E+00   0.000000E+00   0.000000E+00
    INS  5PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00
    INS  6 ICONS   9084.64     -8.58     -2.85
    INS  6BNKPAR    1.3554     154.46     0.00    .00000     .3000    1    1

The annotation of this file is as follows:

.. code-block :: text

    COMM  Y2O3  +  Si  cycle 13/2

Is a comment.

.. code-block :: text

    INS   BANK  6

Tells that this file contains parameters for 6 different banks, where a bank can be mean any collection of detector on an instrument.

.. code-block :: text

    INS   HTYPE   PNTR

Specify histogram type used in obtaining the fitting paramters.
Here PNTR, which is ``P`` for powder data, ``N`` for neutron data, ``T`` for time-of-flight data, and the last letter is a status flag,
``R`` stands for 'read powder data'. Initially we may have a test in Mantid that checks if it is this type and if not tells the user that this is not supported.

.. code-block :: text

    INS  1 ICONS    746.96     -0.24      3.04
    INS  1BNKPAR    2.3696      9.39      0.00    .00000     .3000    1    1
    INS  1I ITYP    0    1.000     25.000       2

These three lines are not relevant to Mantid.

.. code-block :: text

    INS  1PRCF      2   15   0.00100

This tells you which profile function, ``NTYP``, this file contains parameters for.
Here ``NTYP=2``, which is the GSAS Ikeda-Carpenter-pseudo-voigt function described from page 144 in the GSAS Technical Manual.
The following number, ``15``, is the number of parameters of this function and the last number, ``0.00100``, is not relevant to Mantid.

.. code-block :: text

    INS  1PRCF 1   0.000000E+00   0.200000E+00   0.317927E+02   0.544205E+02
    INS  1PRCF 2   0.000000E+00   0.176802E+03   0.000000E+00   0.000000E+00
    INS  1PRCF 3   0.000000E+00   0.000000E+00   0.000000E+00   0.000000E+00
    INS  1PRCF 4   0.000000E+00   0.000000E+00   0.000000E+00

These lines are the actual the 15 GSAS parameters for this profile function.
From page 147 in the GSAS manual these 15 parameters are, in the order listed, the values of:

.. code-block :: text

    "alp-0", "alp-1", "beta",  "switch",
    "sig-0", "sig-1", "sig-2", "gam-0",
    "gam-1", "gam-2", "stec",  "ptec",
    "difc",  "difa" & "zero".

The rest of the file is a repeat for the remaining 5 banks.

Converting GSAS parameters into Mantid parameters
-------------------------------------------------

From comparing the formula for the fitting function :ref:`func-IkedaCarpenterPV` with formulas in the
GSAS Technical Manual the conversion equations are estimated to be:

.. math ::

    Alpha0 = alp \mbox{-}0

.. math ::

    Alpha1 = alp \mbox{-}1

.. math ::

    Beta0 = beta

.. math ::

    Kappa = switch

.. math ::

    SigmaSquared = sig \mbox{-}0 + sig \mbox{-}1 * d^2 + sig \mbox{-}2 * d^4

.. math ::

    Gamma = gam \mbox{-}0 + gam \mbox{-}1 * d + gam \mbox{-}2 * d^2

Where :math:`d` is the dSpacing at the centre of the peak.

Note for now I have ignored GSAS parameters ``stec`` and ``ptec``.
In code it should be checked that these are zero and if not a warning returned to the user.
I believe the remaining three parameter ``difc``, ``difa`` and ``zero`` can be ignored for the exercise here.

The syntax for a Mantid parameter file is described :ref:`here <InstrumentDefinitionFile>`,
in particular look for the section on fitting :ref:`parameter <Using parameter>` on that page.

So the translation of the example above for bank 1 into :ref:`func-IkedaCarpenterPV` parameters gives:

.. code-block :: text

   <parameter name="IkedaCarpenterPV:Alpha0" type="fitting">
     <formula eq="0.0" result-unit="TOF"/>
     <fixed />
   </parameter>
   <parameter name="IkedaCarpenterPV:Alpha1" type="fitting">
     <formula eq="0.200000" result-unit="TOF"/>
     <fixed />
   </parameter>
   <parameter name="IkedaCarpenterPV:Beta0" type="fitting">
     <formula eq="31.7927" result-unit="TOF"/>
     <fixed />
   </parameter>
   <parameter name="IkedaCarpenterPV:Kappa" type="fitting">
     <formula eq="54.4205"/>
     <fixed />
   </parameter>
   <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
     <formula eq="0.0 + 176.802*centre^2 + 0.0*centre^4" unit="dSpacing" result-unit="TOF^2"/>
   </parameter>
   <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
     <formula eq="0.0 + 0.0*centre + 0.0*centre^2" unit="dSpacing" result-unit="TOF"/>
   </parameter>
