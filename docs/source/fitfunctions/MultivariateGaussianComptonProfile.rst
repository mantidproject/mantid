.. _func-MultivariateGaussianComptonProfile:

==================================
MultivariateGaussianComptonProfile
==================================

.. index:: MultivariateGaussianComptonProfile

Description
-----------

The fitted function for y-Space converted values is as described by G.
Romanelli. [1]_.

.. math::
  J(y) = \frac{1}{\sqrt{2\pi} \sigma_{x} \sigma_{y} \sigma_{z}}
         \frac{2}{\pi}
         \int_{0}^{1} d(cos \theta)
         \int_{0}^{\frac{\pi}{2}} d \phi
         S^{2}(\theta, \phi)
         exp
         \left(
           -\frac{y^{2}}
                 {2 S^{2}(\theta, \phi)}
         \right)

Where :math:`S^{2}(\theta, \phi)` is given by:

.. math::
  \frac{1}{S^{2}(\theta, \phi)}
      = \frac{sin^{2}\theta cos^{2}\phi}{\sigma_{x}^{2}}
      + \frac{sin^{2}\theta sin^{2}\phi}{\sigma_{y}^{2}}
      + \frac{cos^{2}\theta}{\sigma_{z}^{2}}

The :math:`A_{3}` Final State Effects (FSE) correction is applied as an additive
correction expressed as:

.. math::

  -A_{3}(q)\frac{d^{3}}{dy^{3}}J(y) =
    \frac{\sigma_{x}^{4} + \sigma_{x}^{4} + \sigma_{x}^{4}}
         {9 \sqrt{2 \pi} \sigma_{x} \sigma_{y} \sigma_{z} q}
    \int_{0}^{1} d(cos \theta)
    \int_{0}^{\frac{\pi}{2}} d \phi
    \left[
      \frac{y^{3}}{S^{2}(\theta, \phi)^{4}}
      -3 \frac{y}{S^{2}(\theta, \phi)^{2}}
    \right]
    S^{2}(\theta, \phi)
    exp
    \left(
      -\frac{y^{2}}
            {2 S^{2}(\theta, \phi)}
    \right)

.. attributes::

   IntegrationSteps;Integer;256;Length of each dimension of integration grid (must be even)

.. properties::

References
----------

.. [1] G. Romanelli, `On the quantum contributions to phase transitions in Water probed by inelastic neutron scattering <https://epubs.stfc.ac.uk/work/12422430>`__

.. categories::

.. sourcelink::
