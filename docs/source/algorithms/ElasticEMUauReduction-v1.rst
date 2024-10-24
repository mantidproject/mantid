.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
The purpose of data reduction is to convert the measured spectra into neutron scattering double differential cross sections.
That is, the scattering probability given a certain momentum exchange between incident neutron and sample.
Together with knowledge of instrument configuration (distances, angles, and backscattering monochromator velocity) for
the detectors, allows assigning momentum transfers to the scattered neutrons, within the instrumental resolution.
Normalisation to the flux profile is further required to obtain a probability.

The Reduce routine thus performs the above necessary algebra on the specified run numbers. At a minimum, the sample runs
are needed. Additional runs for background subtraction and joint detector efficiency / solid angle correction are normally
provided as well, with Reduce applied to each group separately, and the groups combined as shown in the diagram.

The elastic neutron scattering data are typically collected while varying temperature or some other scan parameter over
real time. Therefore Reduce produces cross section and its estimated error vs momentum transfers and chosen scan parameter.

Workflow
--------

.. diagram:: EMUauElasticReduction-v1_wkflw.dot

References
----------

.. sourcelink::

.. categories::
