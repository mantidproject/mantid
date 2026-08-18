.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a workspace with either just a sample or a sample and gauge volume defined and returns an estimation for the centre of mass of the
illuminated region of sample.

Under the hood this algorithm uses the same basic logic as :ref:`algm-AbsorptionCorrection` to rasterize the sample shape
(or any defined gauge volume). It then calculates the average position of all the raster elements which are inside both
shapes, weighting each element by its volume. The weighting matters for shapes that are not divided into equal elements:
a cylinder is divided into annular segments whose volume grows with radius, so an unweighted average would be biased
towards the more finely divided annuli near the axis.

Every element is treated as scattering equally. The illuminated volume is taken to be the gauge volume where one is
defined, and the whole sample otherwise; no account is taken of attenuation, of how the incident intensity varies
across the beam, or of which part of the volume a given detector can see. Where those matter - most notably for a
gauge volume only partly immersed in the sample - see :ref:`algm-WeightedGaugeVolumeAbsorption`, which weights the
same elements by the spatial resolution function and reports a centre of mass per detector.

If the workspace has a goniometer rotation set, the sample shape is interpreted in its own frame and any gauge volume
in the lab frame, and the returned centre of mass is in the lab frame.

.. categories::

.. sourcelink::
