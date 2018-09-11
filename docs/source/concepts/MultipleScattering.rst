.. _Multiple Scattering:

Multiple Scattering
===================

Introduction
~~~~~~~~~~~~
Determination of the structure of samples depends on the analysis of single scattering data. 
Small but unwanted higher-order scattering is always present although in many typical 
experiments multiple scattering effects are negligible. However, in some cases the data may 
contain a significant contribution from multiple scattering. To get an idea of when and why multiple scattering 
corrections are needed let us first define the total cross section per atom as the sum of its 
scattering and absorption cross sections:

.. math::
    
   \sigma_t = \sigma_s + \sigma_a
   
If :math:`\sigma_m` is the likelihood of a neutron being scattered m times then it is possible 
to show [1]_ that:

.. math::

	\sigma_m \sim (\frac{\sigma_s}{\sigma_t})^m
   
Where practical, the shape and thickness of a sample are carefully chosen to minimise as much 
unwanted multiple. This may be achieved by using a sample that is either [2]_

* Small in comparison with its mean free path.
* Strongly absorbing (the absorption cross section is much greater than the scattering cross section. Usually this means the dimensions of a sample are chosen to ensure that between 10% and 20% of incident neutrons end up being scattered [3]_ ).

Increasing the absorption cross section is not always attainable - due to the type of material in question - or desirable, due to 
the accompanying intensity losses becoming overly prohibitive. 

The rest of this document shall explain the theory behind multiple scattering and then outline 
and compare the techniques currently available in Mantid to perform these corrections.

Theory
~~~~~~
The figure shows how a general double scattering process might occur. The neutron travels 
a certain distance :math:`l_1` through the sample before the first scattering event. The second 
scattering occurs after a distance :math:`l_{12}` has been traversed following which the 
neutron travels a final length :math:`l_2` before leaving the sample and being picked up by 
a detector.

.. figure:: ../images/MultipleScatteringVolume.png
   :alt: MultipleScatteringVolume.png

The difficulty in correcting multiple scattering arises from the fact that for each order of scattering
we must perform m volume integrals :math:`dV_1dV_2...dV_m` over the sample to compute the contribution term 
for that order (although these terms tend to zero as explained in the introduction). 
This kind of calculation is incredibly difficult for all but the simplest of geometries 
(i.e. cylindrical, planar and spherical) although Monte Carlo integration 
methods may be utilised for the multiple scattering calculations of more general shapes.

In some areas, such as small angle scattering, there may be useful approximations that can be 
applied that are not present for the more general wide angle scattering case. 
Again matters may become complicated, as for example small angle scatter followed by incoherent 
scatter from hydrogen can be more significant in blurring sharp features than double small angle scatter.
For early considerations of multiple small angle scattering see for example [4]_ [5]_.

Some simplifying approximations can make the calculations somewhat more tractable and the currently
supported Mantid solutions assume that scattering is isotropic and elastic (for something like vanadium this is usually 
reasonable).

Mantid solutions
~~~~~~~~~~~~~~~~

Mayers Sample Correction
------------------------
Documentation for this algorithm can found :ref:`here <algm-MayersSampleCorrection>`.

Carpenter Sample Correction
---------------------------------------
This is a Carpenter style correction. For more details see :ref:`here <algm-CarpenterSampleCorrection>`.

References
~~~~~~~~~~

.. [1] Lindley, E.J., & Mayers, J. Cywinski, R. (Ed.). (1988). Experimental method and corrections to data. United Kingdom: Adam Hilger. - https://inis.iaea.org/search/search.aspx?orig_q=RN:20000574 
.. [2] V.F. Sears (1975): Slow-neutron multiple scattering, `Advances in Physics <http://dx.doi.org/10.1080/00018737500101361>`__, 24:1, 1-45
.. [3] A.K.Soper, W.S.Howells and A.C.Hannon *ATLAS - Analysis of Time-of-Flight Diffraction Data from Liquid and Amorphous Samples* Rutherford Appleton Laboratory Report (1989): `RAL-89-046 <http://wwwisis2.isis.rl.ac.uk/disordered/Manuals/ATLAS/ATLAS%20manual%20v1.0.pdf>`__
.. [4] J.Schelten & W.Schmatz, J.Appl.Cryst. 13(1980)385-390
.. [5] J.R.D.Copley J.Appl.Cryst 21(1988)639-644

.. categories:: Concepts
