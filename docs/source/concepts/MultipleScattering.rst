.. _Multiple Scattering:

Multiple Scattering
===================

Introduction
~~~~~~~~~~~~
Determination of the structure of samples depends on the analysis of single scattering data. 
Small but unwanted higher-order scattering is always present although in many typical 
experiments multiple scattering effects are negligible however in some cases the dataset may 
from significant multiple scattering. To get an idea of when and why multiple scattering 
corrections are needed let us first define the total cross section per atom as the sum of its 
scattering and absorption cross sections:

.. math::
    
   \sigma_t = \sigma_s + \sigma_a
   
If :math:`\sigma_m` is the likelihood of a neutron being scattered m times then it is possible 
to show that:

.. math::

	\sigma_m \sim (\frac{\sigma_s}{\sigma_t})^m
   
Where practical, the shape and thickness of a sample are carefully chosen to minimise as much 
unwanted multiple scattering as possible. We can see from the above relation that this may be 
done most easily either by using a sample that is small in comparison with its mean free path or one that is 
strongly absorbing (the absorption cross section is much greater than the scattering cross 
section). Usually this means the dimensions of a sample are chosen such that no more 
than about 10% of incident neutrons end up being scattered. Increasing the absorption cross 
section is not always attainable, due to the type of material in question, or desirable, due to 
the accompanying intensity losses becoming overly prohibitive. 

The rest of this document shall explain the theory behind multiple scattering and then outline 
and compare the techniques currently available in Mantid to perform these corrections.

Theory
~~~~~~
The figure shows how a generic double scattering process might take place. The neutron travels 
a certain distance :math:`\l_1` through the sample before the first scattering event. The second 
scattering occurs after a distance :math:`\l_{12}` has been traversed following which the 
neutron travels a final length :math:`\l_2` before leaving the sample and being picked up by 
a detector.

.. figure:: ../images/MultipleScatteringVolume.png
   :alt: MultipleScatteringVolume.png

The difficulty in correcting multiple scattering arises from the fact we must integrate both 
:math:`dV_1` and :math:`dV_2` over the entire volume of the sample while accounting for the
scattering and attenuation. This kind of calculation is incredibly difficult for all but the 
simplest of geometries (cylindrical, planar and spherical) although Monte Carlo integration 
methods may be utilised for the multiple scattering calculations of more general shapes.

Some simplifying approximations are usually to assume scattering is isotropic and elastic. 
This makes the calculations somewhat more tractable.

Mantid solutions
~~~~~~~~~~~~~~~~

Mayers Sample Correction
------------------------
Information about the underlying Mantid algorithm can be found in the 
:ref:`MayersSampleCorrection <algm-MayersSampleCorrection>` documentation.

Multiple Scattering Cylinder Absorption
---------------------------------------
Documentation for this algorithm can found within the 
:ref:`MultipleScatteringCylinderAbsorption <algm-MultipleScatteringCylinderAbsorption>` page.

References
~~~~~~~~~~
.. [1] Lindley, E.J., & Mayers, J. Cywinski, R. (Ed.). (1988). Experimental method and corrections to data. United Kingdom: Adam Hilger. - https://inis.iaea.org/search/search.aspx?orig_q=RN:20000574
.. [2] 'ATLAS - Analysis of Time-of-Flight Diffraction Data from Liquid and Amorphous Samples’, A.K.Soper, W.S.Howells and A.C.Hannon, Rutherford Appleton Laboratory Report RAL-89-046. (1989). - http://wwwisis2.isis.rl.ac.uk/disordered/Manuals/ATLAS/ATLAS_manual.htm


.. categories:: Concepts
