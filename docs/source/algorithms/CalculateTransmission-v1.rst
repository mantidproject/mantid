.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the probability of a neutron being transmitted through the
sample using detected counts from two monitors, one in front and one
behind the sample. A data workspace can be corrected for transmission by
`dividing <http://www.mantidproject.org/Divide>`_ by the output of this algorithm.

Because the detection efficiency of the monitors can be different the
transmission calculation is done using two runs, one run with the sample
(represented by :math:`S` below) and a direct run without
it (:math:`D`). The fraction transmitted through the sample :math:`f` is calculated from this formula:

.. math::p = \frac{S_T}{D_T}\frac{D_I}{S_I}

where :math:`S_I` is the number of counts from the monitor in front of
the sample (the incident beam monitor), :math:`S_T` is the transmission
monitor after the sample, etc.

The resulting fraction as a function of wavelength is created as the
OutputUnfittedData workspace. However, because of statistical variations
it is recommended to use the OutputWorkspace, which is the evaluation of
a fit to those transmission fractions. The unfitted data is not affected
by the RebinParams or Fitmethod properties but these can be used to
refine the fitted data. The RebinParams method is useful when the range
of wavelengths passed to CalculateTransmission is different from that of
the data to be corrected.

ChildAlgorithms used
####################

Uses the algorithm :ref:`Fit <algm-Fit>` to fit to the calculated
transmission fraction.

.. categories::

.. sourcelink::
