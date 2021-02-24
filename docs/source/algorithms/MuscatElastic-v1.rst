.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates a Multiple Scattering correction using a Monte Carlo integration method.
The method uses a structure function for the sample to determine the probability of a particular q value for each scattering event and it doesn't therefore rely on an assumption that the scattering is isotropic.

The algorithm is based on code which was originally written in Fortran as part of the Discus program [#JOH]_. The code was subsequently developed by Spencer Howells under the Muscat name.
These original programs calculated multiple scattering corrections for inelastic instruments. This algorithm so far only considers the elastic case.

The algorithm performs the following integration:

:math:`\prod_{i=1}^{N-1} \int_{0}^{l_{i}^{max}}dl_i e^{-\mu_T l_i}\int_{0}^{4 \pi}d \Omega_i^s(\frac{\mu_{coh}}{\mu_s} S(Q_i) + \frac{\mu_{inc}}{\mu_s}) \int_{0}^{l_{n}^{max}}dl_n e^{-\mu_T(l_n + l_{out})}(\frac{\mu_{coh}}{\mu_s} S(Q_i) + \frac{\mu_{inc}}{\mu_s})`

This is similar to the formulation described in [#MAN]_:

:math:`(\frac{\mu_s}{4 \pi})^n \prod_{i=1}^{N-1} \int_{0}^{l_{i}^{max}}dl_i e^{-\mu_T l_i}\int_{0}^{4 \pi}d \Omega_i^s(\frac{\mu_{coh}}{\mu_s} S(Q_i) + \frac{\mu_{inc}}{\mu_s}) \int_{0}^{l_{n}^{max}}dl_n e^{-\mu_T(l_n + l_{out})}(\frac{\mu_{coh}}{\mu_s} S(Q_i) + \frac{\mu_{inc}}{\mu_s})`

The algorithm outputs a workspace group. Each workspace in the group contains a weight for a specific number of scattering events. The number of scattering events ranges between 1 and the number specified in the NumberOfScatterings parameter.
The workspace names are Scatter_n where n is the number of scattering events considered and following the convention in Discus these are referred to as :math:`J_n`.
One additional workspace called Scatter_1_NoAbsorb is also created for a scenario where neutrons are scattered once, absorption is assumed to be zero and re-scattering after the simulated scattering event is assumed to be zero. This is the quantity :math:`J_{1}^{*}` described in the Discus manual [#JOH]_

The output can be applied to a workspace containing a real sample measurement in one of two ways:

- subtraction method. :math:`\sum_{n=2}^{\infty} J_n I_0` can be subtracted from the real sample measurement where :math:`I_0` is the incident intensity
- factor method. The correction can be applied by multiplying the real sample measurement by :math:`J_1/\sum_{n=1}^{\infty} J_n`

The multiple scattering correction should be applied before applying an absorption correction.

The Discus manual describes a further method of applying an attenuation correction and a multiple scattering correction in one step using a variation of the factor method. To achieve this the real sample measurement should be multipled by :math:`J_1^{*}/(\sum_{n=1}^{\infty} J_n`).
Note that this differs from the approach taken in other Mantid absorption correction algorithms such as MonteCarloAbsorption because of the properties of :math:`J_{1}^{*}`.
:math:`J_{1}^{*}` corrects for attenuation due to absorption before and after the simulated scattering event (which is the same as MonteCarloAbsorption) but it only corrects for attenuation due to scattering after the simulated scattering event.
For this reason it's not clear this feature from Discus is useful but it has been left in for historical reasons.

The sample shape can be specified by running the algorithm SetSample on the input workspace prior to running this algorithm.


References
##########

.. [#JOH] M W Johnson, AERE Report R7682 (1974)
.. [#MAN] R Mancinelli, Multiple neutron scattering corrections. Some general equations to do fast evaluations (2012)


Usage
----- 


.. categories::

.. sourcelink::
