.. _CreateBackToBackParameters:

Create Back To Back Parameters
==============================

.. contents:: Table of Contents
  :local:

Introduction
------------

This is how to create back-to-back exponential peudo-voigt Fullprof fit function parameters with Mantid's :ref:`BackToBackExponential<func-BackToBackExponential>` function.

Example Fullprof HRPD ICPV instrument parameter file
----------------------------------------------------

The fullprof param file (also called fullprof resolution file) below describes default fitting parameters to use for the fullprof NPROF=9 fitting functions. More specifically it does this in the example below for three collections of detectors, which could be any collection of detectors, but for this particular example happens to be:

- The 1st set of parameters refer to the detectors of the bank scattering bank. In HPRD_Definition.xml called "bank_bsk"
- The 2nd set of parameters refer to the detectors of the 90 degree bank. In HPRD_Definition.xml called "bank_90degnew"
- The 3nd set of parameters refer to the collection of detectors grouped with the name "bank_1a" HPRD_Definition.xml called "bank_90degnew"

.. code-block:: text

    Instrumental resolution function for HRPD/ISIS L. Chapon 12/2003  ireso: 5
    ! To be used with function NPROF=9 in FullProf (Res=5)
    ! ----------------------------------------------------  Bank 1
    !  Type of profile function: back-to-back expon * pseudo-Voigt
    NPROF   9
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   10364.0000      1.0000   235100.00000
    !        Dtt1           Dtt2       Zero
    D2TOF     48293.168           -0.201        -2.086
    !     TOF-TWOTH of the bank
    TWOTH    168.33
    !       Sig-2     Sig-1     Sig-0
    SIGMA   7.429     132.188      0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     7.550   0.000
    !          alph0       beta0       alph1       beta1
    ALFBE    0.000000    0.026707    0.086999    0.005560
    END
    ! ----------------------------------------------------- Bank 2
    !  Type of profile function: back-to-back expon * pseudo-Voigt
    NPROF   9
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   10680.00     7.85    231500.00
    !        Dtt1          Dtt2        Zero
    D2TOF     34824.055           3.354        8.729
    !     TOF-TWOTH of the bank
    TWOTH    89.58
    !       Sig-2       Sig-1     Sig-0
    SIGMA   158.011     4166.279     0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     4.916     0.000
    !          alph0       beta0       alph1       beta1
    ALFBE    0.000000    0.020300    0.086999    0.019136
    END
    ! ----------------------------------------------------- Bank 2
    !  Type of profile function: back-to-back expon * pseudo-Voigt
    NPROF   9
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   10570.00     7.85    231500.00
    !        Dtt1          Dtt2        Zero
    D2TOF     12763.054           8.302        0.000
    !     TOF-TWOTH of the bank
    TWOTH    89.58
    !       Sig-2       Sig-1     Sig-0
    SIGMA   188.149     6520.945     0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     0.000     0.000
    !          alph0       beta0       alph1       beta1
    ALFBE    0.000000    0.026707    0.086999    0.005560
    END

Converting above Fullprof ICPV parameter into Mantid parameter file
-------------------------------------------------------------------

The syntax for a Mantid parameter file is described :ref:`here <InstrumentDefinitionFile>`, in particular look for the section on fitting :ref:`parameter <Using parameter>` on that page.

Fullprof definition of ``NPROF=9`` is the convolution of a back-to-back exponential with a pseudo-voigt.
This is in fact not a perfect match with Mantid's :ref:`BackToBackExponential<func-BackToBackExponential>` function
(as of this writing), since it is the peak shape function for a back-to-back exponential convoluted with a Gaussian.
Hence the Lorentzian part of the fullprof ``NPROF=9`` is here ignored.

So the exercise is to convert fullprof

.. code-block:: text

    !       Sig-2     Sig-1     Sig-0
    SIGMA   7.429     132.188      0.000
    !          alph0       beta0       alph1       beta1
    ALFBE    0.000000    0.026707    0.086999    0.005560

Into the parameters :math:`A`, :math:`B` and :math:`S` of :ref:`BackToBackExponential<func-BackToBackExponential>`.
From comparing the formula for the fitting function :ref:`BackToBackExponential<func-BackToBackExponential>` with
formulas in the `Fullprof manual <http://www.ill.eu/sites/fullprof/>`_ the conversion equations are estimated to be:

.. math::

    S = \sqrt{Sig \mbox{-}2 * d^4 + Sig\mbox{-}1 * d^2 + Sig\mbox{-}0}

.. math::

    A = alph0 + alph1/d

.. math::

    B = beta0 + beta1/d^4

where :math:`d` is the d-Spacing at the centre of the peak.

So the translation of the example just above into :ref:`BackToBackExponential<func-BackToBackExponential>` parameters gives:

.. code-block:: text

      <parameter name="BackToBackExponential:S" type="fitting">
        <formula eq="sqrt(7.429*centre^4+132.188*centre^2)" unit="dSpacing" result-unit="TOF" />
      </parameter>
      <parameter name="BackToBackExponential:A" type="fitting">
        <formula eq="(0.086999/centre)" unit="dSpacing" result-unit="1/TOF" /> <fixed />
      </parameter>
      <parameter name="BackToBackExponential:B" type="fitting">
        <formula eq="(0.026707+0.005560/(centre^4))" unit="dSpacing" result-unit="1/TOF" /> <fixed />
      </parameter>

Notice ``<fixed />`` has been added, such that, by default the parameters ``A`` and ``B`` are fixed.
This is entirely optional, but for fitting e.g. HRPD or GEM data this makes sense since these parameters are supposed
to be instrument specific (considered fixed for a given beamline period at least) whereas ``S`` depends on the specific
sample data that are collected on the beamline.

In Fullprof: Sig-2, Sig-1, beta1, etc. carries units and the result-unit of ``S`` is TOF and the result-unit of ``A`` and ``B`` is 1/TOF.

.. categories:: Techniques
