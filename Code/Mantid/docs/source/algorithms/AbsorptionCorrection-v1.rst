.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm uses a numerical integration method to calculate
attenuation factors resulting from absorption and single scattering in a
sample with the material properties given. Factors are calculated for
each spectrum (i.e. detector position) and wavelength point, as defined
by the input workspace. The sample is first bounded by a cuboid, which
is divided up into small cubes. The cubes whose centres lie within the
sample make up the set of integration elements (so you have a kind of
'Lego' model of the sample) and path lengths through the sample are
calculated for the centre-point of each element, and a numerical
integration is carried out using these path lengths over the volume
elements.

Note that the duration of this algorithm is strongly dependent on the
element size chosen, and that too small an element size can cause the
algorithm to fail because of insufficient memory.

Note that The number density of the sample is in
:math:`\mathrm{\AA}^{-3}`

Choosing an absorption correction algorithm
-------------------------------------------

This flow chart is given as a way of selecting the most appropriate of
the absorption correction algorithms. It also shows the algorithms that
must be run first in each case. Note that this does not cover the
following absorption correction algorithms:
:ref:`algm-MonteCarloAbsorption` (correction factors for
a generic sample using a Monte Carlo instead of a numerical integration
method),
:ref:`algm-MultipleScatteringCylinderAbsorption`
& :ref:`algm-AnvredCorrection` (corrections in a spherical
sample, using a method imported from ISAW). Also, HRPD users can use the
:ref:`algm-HRPDSlabCanAbsorption` to add rudimentary
calculations of the effects of the sample holder. |AbsorptionFlow.png|

Assumptions
###########

This algorithm assumes that the (parallel) beam illuminates the entire
sample **unless** a 'gauge volume' has been defined using the
:ref:`algm-DefineGaugeVolume` algorithm (or by otherwise
adding a valid XML string `defining a
shape <HowToDefineGeometricShape>`__ to a `Run <Run>`__ property called
"GaugeVolume"). In this latter case only scattering within this volume
(and the sample) is integrated, because this is all the detector can
'see'. The full sample is still used for the neutron paths. (**N.B.** If
your gauge volume is of axis-aligned cuboid shape and fully enclosed by
the sample then you will get a more accurate result from the
:ref:`algm-CuboidGaugeVolumeAbsorption`
algorithm.)

Restrictions on the input workspace
###################################

The input workspace must have units of wavelength. The
`instrument <instrument>`__ associated with the workspace must be fully
defined because detector, source & sample position are needed.

.. |AbsorptionFlow.png| image:: /images/AbsorptionFlow.png

.. categories::
