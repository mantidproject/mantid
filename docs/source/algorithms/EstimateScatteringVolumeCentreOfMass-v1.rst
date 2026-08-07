.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a workspace with either just a sample or a sample and gauge volume defined and returns an estimation for the centre of mass of the
illuminated region of sample.

Under the hood this algorithm uses the same basic logic as :ref:`algm-AbsorptionCorrection` to rasterize the sample shape
(or any defined gauge volume). It then calculates the average position of all the raster elements which are inside both shapes.

Neutron weighting
-----------------

By default every volume element counts equally, giving the purely geometric centre of the
illuminated region. Setting ``UseNeutronWeightings`` instead weights each element by how much
signal it actually contributes, which is the spatial resolution function

.. math:: SRF(r) = P_i(r) \cdot P_s(r) \cdot P_d(r)

where :math:`P_i` is the incident beam profile, :math:`P_s` the probability of a neutron
scattering at that point and reaching the detector, and :math:`P_d` the collimator acceptance.
This is the quantity Creek, Santisteban & Edwards (2005) call the neutron-weighted centre of
gravity, and it differs from the geometric centre whenever attenuation is significant or the
gauge volume is only partly immersed in the sample.

The three terms come from:

- :math:`P_i` - the beam profile set by :ref:`algm-SetBeam`. Without one the whole sample is
  assumed to be uniformly illuminated.
- :math:`P_s` - attenuation along the incoming and outgoing paths through the sample, summed
  over the workspace's wavelength points. The sample must therefore have a material, and the
  workspace must be in units of Wavelength.
- :math:`P_d` - a Gaussian collimator acceptance, applied only when the instrument carries a
  ``col-gauge-width`` parameter giving the collimator's calibrated gauge width in metres.
  Without it no collimator restriction is applied.

Because the outgoing path length differs between detectors, so does the resulting centre of
mass. ``DetectorScatteringCentres`` optionally returns those per-detector centres as a table of
detector ID, position and total weight; detectors that see nothing of the scattering volume are
omitted. The scalar ``CentreOfMass`` output is then the mean of those centres weighted by how
much each detector sees, rather than a plain average.

Choosing an element size
------------------------

``ElementSize`` controls the rasterisation. The unweighted calculation costs one operation per
element, but the weighted calculation traces a ray from every element to every detector, so the
cost grows as elements times detectors. A 0.5 mm element on a 4 mm gauge volume is a reasonable
starting point; much finer settings become very slow on a full instrument. Too coarse a grid
biases the centre near a sample surface, so check that halving the element size does not move
the result significantly.

.. categories::

.. sourcelink::
