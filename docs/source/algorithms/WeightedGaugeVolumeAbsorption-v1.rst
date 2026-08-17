.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates attenuation factors in the same way as :ref:`algm-AbsorptionCorrection`, but weights each
integration element by how strongly it contributes to what a given detector actually measures,
rather than treating every element as scattering equally.

:ref:`algm-AbsorptionCorrection` assumes the whole sample is uniformly illuminated and that every
detector sees all of it. That is a fair description of a powder measurement, but not of a strain or
texture measurement, where the illuminated region is a small gauge volume shaped by incident slits
and each detector views that region through a radial collimator.

The weight applied to each element is the spatial resolution function of
Creek, Santisteban & Edwards (2005),

.. math:: \mathrm{SRF}(\mathbf{r}) = P_i(\mathbf{r})\, P_s(\mathbf{r})\, P_d(\mathbf{r})

where :math:`P_i` is the incident beam profile, :math:`P_s` the attenuation along the incoming and
outgoing paths, and :math:`P_d` the collimator acceptance. The attenuation factor reported for
spectrum :math:`i` at wavelength :math:`\lambda` is

.. math::
   A_i(\lambda) = \frac{\sum_e V_e\, P_i(\mathbf{r}_e)\, P_d(\mathbf{r}_e, i)\,
                        e^{-\mu(\lambda)(L1_e + L2_{ei})}}
                       {\sum_e V_e\, P_i(\mathbf{r}_e)\, P_d(\mathbf{r}_e, i)}

so that it remains a factor between zero and one, tending to one as the attenuation vanishes. Only
:math:`P_d` depends on the detector, and it is what makes the effective scattering volume - and
hence the apparent scattering centre - differ from detector to detector.

Inputs
######

The integration volume is the gauge volume when one has been defined with
:ref:`algm-DefineGaugeVolume`, and the whole sample otherwise. Neither weighting term has to be
present:

- Without beam geometry set by :ref:`algm-SetBeam`, the illumination is treated as uniform. This is
  the usual case when a gauge volume is defined, since the gauge already describes the lit region.
- Without a calibrated ``col-gauge-width`` instrument parameter, the collimator acceptance is
  ignored.

With neither present, every weight is unity and this algorithm returns exactly the same values as
:ref:`algm-AbsorptionCorrection`.

Outputs
#######

``OutputWorkspace`` holds the attenuation factors; divide your data by them to apply the correction.

``IlluminatedVolumeFraction`` is optional and single-valued per spectrum: the fraction of the whole
sample volume that is both illuminated and visible to that detector. Multiplying it by
``OutputWorkspace`` normalises the correction by the entire sample rather than by the volume the
detector sees, which is the appropriate choice when the change in scattering volume matters - for
instance as a gauge volume is scanned out of a sample.

``ScatteringCentres`` is an optional table of the neutron weighted centre of gravity of the volume
each detector sees, in the lab frame and in metres, with columns ``detid``, ``x``, ``y``, ``z`` and
``weight``. For a gauge volume fully immersed in a weakly attenuating sample this sits at the
geometric centre; for one only partly immersed it is displaced back inside the sample, which is the
leading cause of pseudo-strain in near-surface measurements. Where a spectrum groups several
detectors, its weight is divided between them so that summing the column recovers the total.

Choosing an element size
########################

The integration is a midpoint rule over cubic elements, so it is accurate while
:math:`\mu h \ll 1`, with :math:`h` the element size. The 1 mm default is a reasonable starting
point for most metals; halving it and confirming the result is stable is the practical check. Note
that the run time grows as the cube of the reciprocal element size.

Compared with the alternatives
##############################

- :ref:`algm-AbsorptionCorrection` performs the same quadrature without the beam or collimator
  weighting, and reports no scattering centres.
- :ref:`algm-CuboidGaugeVolumeAbsorption` requires the sample to *fully enclose* an axis-aligned
  cuboid gauge centred on the sample position. This algorithm accepts any gauge shape at any
  position, including one only partly immersed in the sample.
- :ref:`algm-MonteCarloAbsorption` handles containers and sample environments, which this algorithm
  does not, and multiple scattering configurations that a single-scatter analytical treatment
  cannot. In exchange this algorithm is deterministic and free of the statistical noise that would
  otherwise propagate into any quantity derived from the scattering centres.

Restrictions and assumptions
############################

- Single scattering only.
- Attenuation is computed through the sample alone; containers and sample environments are ignored.
- The input workspace must have units of wavelength and a fully defined instrument.
- Cost scales as the number of detectors multiplied by the number of elements, so a whole-sample
  integration at a small element size over many detectors can be slow. Defining a gauge volume is
  the usual way to keep this in hand.

Usage
-----

**Example: a gauge volume partly withdrawn from a sample**

.. testcode:: ExGaugeVolume

    ws = CreateSampleWorkspace("Histogram", NumBanks=1, BankPixelWidth=1)
    ws = ConvertUnits(ws, "Wavelength")
    ws = Rebin(ws, Params=[1])

    CreateSampleShape(ws, '''<cuboid id="sample">
      <height val="0.02" /><width val="0.02" /><depth val="0.02" />
      <centre x="0.0" y="0.0" z="0.0" />
      </cuboid><algebra val="sample" />''')
    SetSampleMaterial(ws, ChemicalFormula="Fe")

    DefineGaugeVolume(ws, '''<cuboid id="gauge">
      <height val="0.004" /><width val="0.004" /><depth val="0.004" />
      <centre x="0.009" y="0.0" z="0.0" />
      </cuboid><algebra val="gauge" />''')

    WeightedGaugeVolumeAbsorption(ws, OutputWorkspace="factors", ElementSize=0.5,
                                  ScatteringCentres="centres")
    centres = mtd["centres"]

    # The sample surface is at x = 0.01 m, so the far half of the gauge volume is outside it and
    # the volume that actually scatters is the near half, centred at 0.0085 m.
    print("Gauge volume aimed at    x = 0.0090 m")
    print("Scattering centre is at  x = {:.4f} m".format(centres.cell("x", 0)))

Output:

.. testoutput:: ExGaugeVolume

    Gauge volume aimed at    x = 0.0090 m
    Scattering centre is at  x = 0.0085 m

.. categories::

.. sourcelink::
