.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to reduce powder diffraction data from the HFIR instruments at ORNL. It is a
workflow algorithm that uses several other algorithms to perform the reduction. The input data can be
specified either as a list of files or as an IPTS number and a list of run numbers. This works for both WAND and MIDAS instruments.
Reducing WAND^2 data with HFIRPowderReduction is very similar to using :ref:`algm-WANDPowderReduction`, but with some differences in the input parameters,
and reduction steps are slightly modified.

By default the instrument geometry is determined by the sample file. An instrument definition file (IDF) can
optionally be supplied through the ``IDFFilename`` property to override the geometry used by the sample file.

Reduction formula
-----------------

Four inputs take part in the reduction: the sample :math:`S`, the sample background
:math:`S_B`, the vanadium :math:`V` and the vanadium background :math:`V_B`. Each one is
normalised by its **own** monitor count or counting time, so that all four are on a
common per-unit-exposure scale before anything is subtracted or divided.

Normalisation
#############

For a workspace :math:`X` the normalisation scale :math:`c_X` is chosen by ``NormaliseBy``:

.. math::

   c_X = \begin{cases}
   1                          & \texttt{None} \\
   \texttt{gd\_prtn\_chrg}    & \texttt{Monitor} \quad \text{(the integrated monitor count)} \\
   \texttt{duration}          & \texttt{Time}
   \end{cases}

Throughout this section a hat denotes the normalised workspace,
:math:`\hat{X} = X / c_X`. ``NormaliseBy`` defaults to ``Monitor`` for WAND² and to
``Time`` for MIDAS; with ``None`` every scale is 1 and the expressions below reduce to
raw counts.

Vanadium calibration
####################

The vanadium background is subtracted from the vanadium, and the result is corrected for
absorption and multiple scattering:

.. math::

   \hat{V}_\mathrm{corr} = \left( \hat{V} - \hat{V}_B \right) \frac{1 - \Delta_V}{A_V}

Sample
######

The sample background is scaled by ``SampleBackgroundScaleFactor`` (:math:`f_B`) and
subtracted, and the result is corrected for absorption and multiple scattering:

.. math::

   \hat{S}_\mathrm{corr} = \left( \hat{S} - f_B \hat{S}_B \right) \frac{1 - \Delta_S}{A_S}

Because both terms are normalised first, the subtraction is independent of the relative
exposure of the sample and background runs.

Output
######

.. math::

   S_\mathrm{out} = s \, f_\mathrm{norm} \, \frac{\hat{S}_\mathrm{corr}}{\hat{V}_\mathrm{corr}}

where :math:`s` is the ``Scale`` property (default 1) and :math:`f_\mathrm{norm}` is the
absolute-intensity factor described below (:math:`f_\mathrm{norm} = 1` unless
``AbsoluteIntensityUnits`` is set).

Any input that is not supplied simply drops out of the expression: with no vanadium the
division by :math:`\hat{V}_\mathrm{corr}` is skipped, and with no background the
corresponding subtraction is skipped.

.. list-table::
   :header-rows: 1
   :widths: 12 88

   * - Symbol
     - Property
   * - :math:`S`
     - ``SampleFilename``, or ``SampleIPTS`` and ``SampleRunNumbers``
   * - :math:`S_B`
     - ``SampleBackgroundFilename``, or ``SampleBackgroundIPTS`` and ``SampleBackgroundRunNumbers``
   * - :math:`V`
     - ``VanadiumFilename``, or ``VanadiumIPTS`` and ``VanadiumRunNumbers``
   * - :math:`V_B`
     - ``VanadiumBackgroundFilename``, or ``VanadiumBackgroundIPTS`` and ``VanadiumBackgroundRunNumbers``
   * - :math:`c_X`
     - ``NormaliseBy``
   * - :math:`f_B`
     - ``SampleBackgroundScaleFactor``
   * - :math:`s`
     - ``Scale``

Absorption and multiple scattering
##################################

:math:`A` (absorption) and :math:`\Delta` (multiple scattering factor) are computed with
:ref:`algm-CylinderAbsorptionCW` using the ``Sabine`` method, at the wavelength given by
``Wavelength``.

For the vanadium, the correction is applied only when ``VanadiumDiameter`` is greater than
zero; otherwise :math:`A_V = 1` and :math:`\Delta_V = 0`, so
:math:`\hat{V}_\mathrm{corr} = \hat{V} - \hat{V}_B`. The rod is modelled as vanadium metal
at 6.1172 g/cm³ with radius ``VanadiumDiameter`` / 2 and height ``VanadiumHeight``.

For the sample, the correction is applied only when ``DoAttenuationCorrection`` is set;
otherwise :math:`A_S = 1` and :math:`\Delta_S = 0`. The sample is modelled with
``SampleChemicalFormula`` at a mass density of
``SampleCrystalDensity`` × ``SamplePackingFraction``, with radius ``SampleDiameter`` / 2 and
height ``SampleHeight``. Multiple scattering is included only when
``DoMultipleScatteringCorrection`` is also set; when it is not, :math:`\Delta_S = 0` while
the absorption correction :math:`A_S` still applies.

Absolute intensity units
########################

When ``AbsoluteIntensityUnits`` is set the output is put on an absolute scale of
mb/sr/formula unit by comparing the scattering power of the sample cylinder with that of
the vanadium cylinder:

.. math::

   f_\mathrm{norm} = \frac{1000}{4\pi} \,
   \frac{M_S \, \sigma_V \, \rho_V \, h_V \, r_V^2}
        {M_V \, f_S \, \rho_S \, h_S \, r_S^2}

with :math:`\sigma_V = 5.08` barn (total scattering cross-section of vanadium),
:math:`\rho_V = 6.1172` g/cm³ and :math:`M_V = 50.94` g/mol. :math:`M_S` is the relative
molecular mass of ``SampleChemicalFormula``, :math:`\rho_S` is ``SampleCrystalDensity``,
:math:`f_S` is ``SamplePackingFraction``, and :math:`h` and :math:`r` are the heights and
radii of the vanadium and sample cylinders as above. This requires
``VanadiumDiameter``, ``VanadiumHeight`` and ``SampleHeight`` to all be greater than zero.

.. categories::

.. sourcelink::
