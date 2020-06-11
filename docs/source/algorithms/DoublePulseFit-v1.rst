.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is is designed to analyse double pulse Muon data. It fits two copies of the input function seperated by a time offset. The function fitted is of the form

.. math::
  f(t) = N_1 f(t + t_s)_{input} + N_2 f(t)_{input}.

Where :math:`N_1` is the FirstPulseWeight, :math:`N_2` is the SecondPulseWeight and :math:`t_s` is the PulseOffset. The parameters of the two input functions are all tied. In practise this is achieved by taking a convolution of two delta functions with the input function.

.. math::
  f(t) = f(t)_{input} * (N_1\delta(-ts) + N_2\delta(0)).

With the exception of the three new parameters PulseOffset, FirstPulseWeight and SecondPulseWeight the interface for DoublePulseFit is identical to that of Fit and the two can be used interchangeably.
The parameters and function output by this algorithm corrospond to the inner function only whilst the output fit curves corrospond to the full function.

