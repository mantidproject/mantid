.. _CreateIkedaCarpenterParametersFullprof:

Create Ikeda Carpenter Parameters Fullprof
==========================================

.. contents:: Table of Contents
  :local:

Introduction
------------

This is how to take a Fullprof parameter file containing :ref:`Ikeda-Carpenter-Pseudo-Voigt <func-IkedaCarpenterPV>` parameters and translate
these into equivalent parameters for the Mantid implementation of this function.

Example Fullprof GEM ICPV instrument parameter file
---------------------------------------------------

.. code-block :: text

    Instrumental resolution function for GEM/ISIS L. Chapon 11/2003  ireso: 5
    ! To be used with function NPROF=13 in FullProf  (Res=5)
    ! ----------------------------------------------------  Bank 1
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   1200.0000      1.0000   19000
    !        Dtt1      Dtt2         Zero
    D2TOF  793.406     0.000       0.364
    !     TOF-TWOTH of the bank
    TWOTH    9.39
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000  166.868     98.757
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     0.277     0.000
    !          alph0       beta0       alph1       kappa
    ALFBE      0.734079   32.017204    2.067249   48.734158
    END
    ! ----------------------------------------------------- Bank 2
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   1500.0000      1.0000  19000.
    !        Dtt1      Dtt2         Zero
    D2TOF  1476.065    0.011      1.881
    !     TOF-TWOTH of the bank
    TWOTH    17.98
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000  263.595  0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     3.450   0.000
    !          alph0       beta0       alph1       kappa
    ALFBE      0.734079   32.017204    2.067249   48.734158
    END
    ! ----------------------------------------------------- Bank 3
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   2000.0000      1.0000  19000
    !        Dtt1      Dtt2          Zero
    D2TOF  2798.554   -0.274       -1.621
    !     TOF-TWOTH of the bank
    TWOTH    34.96
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000   287.456   0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     3.645    0.000
    !          alph0       beta0       alph1       kappa
    ALFBE      0.734079   32.017204    2.067249   48.734158
    END
    ! ----------------------------------------------------- Bank 3
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   2800.0000      1.0000  19000
    !        Dtt1      Dtt2           Zero
    D2TOF  4869.121    -2.612       -4.127
    !     TOF-TWOTH of the bank
    TWOTH    63.62
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000   176.833    0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     4.416     0.000
    !          alph0       beta0       alph1       kappa
    ALFBE     0.734079   32.017204    2.067249   48.734158
    END
    ! ----------------------------------------------------- Bank 3
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   3300.0000      1.0000  19000
    !        Dtt1      Dtt2           Zero
    D2TOF  6671.694   -5.778        -5.029
    !     TOF-TWOTH of the bank
    TWOTH    91.30
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000    63.410   0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000      3.116   0.000
    !          alph0       beta0       alph1       kappa
    ALFBE      0.734079   32.017204    2.067249   48.734158
    END
    ! ----------------------------------------------------- Bank 3
    !  Type of profile function: Ikeda-Carpenter * pseudo-Voigt
    NPROF   13
    !       Tof-min(us)    step      Tof-max(us)
    TOFRG   4500.0000      1.0000  16700
    !        Dtt1      Dtt2          Zero
    D2TOF  9077.306    -11.374     -6.370
    !     TOF-TWOTH of the bank
    TWOTH    154.40
    !       Sig-2     Sig-1     Sig-0
    SIGMA    0.000   29.321  0.000
    !       Gam-2     Gam-1     Gam-0
    GAMMA   0.000     0.982   0.000
    !          alph0       beta0       alph1       kappa
    ALFBE     0.734079   32.017204    2.067249   48.734158
    END

Converting above Fullprof ICPV parameter into Mantid parameter file
-------------------------------------------------------------------

The syntax for a Mantid parameter file is described :ref:`here <InstrumentDefinitionFile>`.

The Fullprof Ikeda Carparter parameters of the above Fullprof file is the four numbers stored to the right of ALFBE.
These are the same for all the banks and in Mantid these are therefore assigned to the top level component of the GEM instrument (see ``GEM_Definition.xml``),
which is called GEM, and Mantid representation of Alpha0, Beta0, Alpha1 and Kappa are:

.. code-block :: text

  <component-link name="GEM" >
    <parameter name="IkedaCarpenterPV:Alpha0" type="fitting">
      <formula eq="0.734079" result-unit="TOF"/>
      <fixed />
    </parameter>
    <parameter name="IkedaCarpenterPV:Beta0" type="fitting">
      <formula eq="32.017204" result-unit="TOF"/>
      <fixed />
    </parameter>
    <parameter name="IkedaCarpenterPV:Alpha1" type="fitting">
      <formula eq="2.067249" result-unit="TOF"/>
      <fixed />
    </parameter>
    <parameter name="IkedaCarpenterPV:Kappa" type="fitting">
      <formula eq="48.734158"/>
      <fixed />
    </parameter>
  </component-link>

Notice ``<fixed />`` has been added to each of these 4 parameters. This means that by default the Mantid fitting will treat these parameters as fixed.

Also notice, the ``result-unit="TOF"`` tag for 3 of the parameters. The ``result-unit`` is the unit of the parameter, see :ref:`IDF <InstrumentDefinitionFile>`.
The advantage of specifying units of the parameters is that Mantid will then automatically be able to convert the specified
parameter values to be correct independently of whether the workspace is for instance in units of dSpacing or TOF.
For the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak-shape function setting these units has one additional complication.
From the formulas on page :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` it is seen that this peak shape function explicitly depends on both the time-of-flight (:math:`t`) and wavelength (:math:`\lambda`).

The units for ``Alpha0``, ``beta0``, ``Alpha1`` and ``Kappa`` are: :math:`TOF`, :math:`TOF`, :math:`\frac{TOF}{Wavelength}` and :math:`\frac{1}{Wavelength^2}` respectively, see :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>`.
However, the :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` algorithm calculates the value of (:math:`\lambda`) separately and to the same unit regardless of what the workspace unit is.
Therefore for :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` to ensure ``Alpha1`` and ``Kappa`` works regardless of the workspace unit the result-unit is assigned
for these to TOF and dimensionless respectively (i.e. ignoring the wavelength bit of the unit).

``SIGMA`` in the Fullprof file is the sigma pseudo-voigt parameter. For the 1st bank it is ``SIGMA 0.000 166.868 98.757``,
and it can be found to translates into the equation: :math:`variance=SigmaSquared = Sig \mbox{-}2 * d^4 + Sig \mbox{-}1 * d^2 + Sig \mbox{-}0`,
where variance is in units of :math:`TOF^2`, and :math:`d` is the dSpacing at the centre of the peak.
This translates into the Mantid parameter:

.. code-block :: text

    <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
      <formula eq="0.0*centre^4+166.868*centre^2+98.757" unit="dSpacing" result-unit="TOF^2"/>
    </parameter>

Note for illustration purpose only ``0.0*centre^4`` has been added to the parameter formula equation above.
Note in addition to using the result-unit attribute this formula also assigns ``unit="dSpacing"``, which is the unit of ``centre`` variable in the formula.

``GAMMA`` in the Fullprof file is the gamma pseudo-voigt parameter.
For the 1st bank it is ``GAMMA 0.000 0.277 0.000``, and it can be found to translate into the equation:
:math:`GAMMA=Gamma = Gam \mbox{-}2 * d^2 + Gam \mbox{-}1 * d + Gam \mbox{-}0`, where ``Gamma`` is in units of TOF and :math:`d` is the dSpacing at the centre of the peak.
This translates into the Mantid parameter:

.. code-block :: text

    <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
      <formula eq="0.0*centre^2+0.277*centre+0.0" unit="dSpacing" result-unit="TOF"/>
    </parameter>

A copy of the full translation of the above Fullprof instrument file into a Mantid parameter file is shown below
(this parameter file can be applied to a GEM workspace with :ref:`algm-LoadParameterFile` or by copying in into ``MantidInstall/instrument/GEM_Parameters.xml``):

.. code-block :: text

    <?xml version="1.0" encoding="UTF-8" ?>
    <parameter-file instrument="GEM" date = "2003-11-30 23:59:59">

      <component-link name="GEM" >
        <parameter name="IkedaCarpenterPV:Alpha0" type="fitting">
          <formula eq="0.734079" result-unit="TOF"/>
          <fixed />
        </parameter>
        <parameter name="IkedaCarpenterPV:Beta0" type="fitting">
          <formula eq="32.017204" result-unit="TOF"/>
          <fixed />
        </parameter>
        <parameter name="IkedaCarpenterPV:Alpha1" type="fitting">
          <formula eq="2.067249" result-unit="TOF"/>
          <fixed />
        </parameter>
        <parameter name="IkedaCarpenterPV:Kappa" type="fitting">
          <formula eq="48.734158"/>
          <fixed />
        </parameter>
      </component-link>

      <component-link name="bank1" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="166.868*centre^2+98.757" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
          <formula eq="0.277*centre" unit="dSpacing" result-unit="TOF"/>
        </parameter>
      </component-link>

      <component-link name="bank2" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="263.595*centre^2" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
          <formula eq="3.45*centre" unit="dSpacing" result-unit="TOF"/>
        </parameter>
      </component-link>

      <component-link name="bank3" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="287.456*centre^2" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
          <formula eq="3.645*centre" unit="dSpacing" result-unit="TOF"/>
        </parameter>
      </component-link>

      <component-link name="bank4" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="176.833*centre^2" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting" result-unit="TOF">
          <formula eq="4.416*centre" unit="dSpacing"/>
        </parameter>
      </component-link>

      <component-link name="bank5" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="63.41*centre^2" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
          <formula eq="3.116*centre" unit="dSpacing" result-unit="TOF"/>
        </parameter>
      </component-link>

      <component-link name="bank6" >
        <parameter name="IkedaCarpenterPV:SigmaSquared" type="fitting">
          <formula eq="29.321*centre^2" unit="dSpacing" result-unit="TOF^2"/>
        </parameter>
        <parameter name="IkedaCarpenterPV:Gamma" type="fitting">
          <formula eq="0.982*centre" unit="dSpacing" result-unit="TOF"/>
        </parameter>
      </component-link>

    </parameter-file>
