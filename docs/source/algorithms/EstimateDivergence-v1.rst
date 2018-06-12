
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm estimates the divergence of a diffraction instrument using equation 6.9 of Windsor

.. math:: \Delta\theta_{div} = \frac{1}{2}
          \sqrt{\Delta(2\theta)^2 + \alpha_0
          + \frac{4\left(\beta_0^2 + \beta_1^2\right)}{\sin^2(2\theta)}}

Where :math:`\Delta\theta_{div}` is the divergence, :math:`\Delta(2\theta)` is the angular uncertainty due to the detector size, :math:`\alpha_0` is the uncertainty in the incident collimation in the scattering plane, and the :math:`\beta` terms are the angular uncertainties out of the scattering plane for the incident and scattered beam.

The results of this calculation can be supplied as an optional workspace to :ref:`EstimateResolutionDiffraction <algm-EstimateResolutionDiffraction>`.


Usage
-----

**Example - EstimateDivergence**

.. testcode:: EstimateDivergenceExample

   LoadEmptyInstrument(Filename='POWGEN_Definition_2017-05-01.xml', OutputWorkspace='PG3')
   ws = EstimateDivergence(InputWorkspace='PG3')

   # Print the result
   print("The output workspace has {} spectra".format(ws.getNumberHistograms()))

Output:

.. testoutput:: EstimateDivergenceExample

  The output workspace has 43121 spectra

References
----------

#. Windsor, C. G. *Pulsed Neutron Scattering.* London: Taylor & Francis, 1981. Print. ISBN-10: 0470271310, ISBN-13: 978-0470271315

.. seealso :: Algorithm :ref:`algm-EstimateResolutionDiffraction`

.. categories::

.. sourcelink::
