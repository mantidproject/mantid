.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is is designed to analyse double pulse Muon data. It fits two copies of the input function seperated by a time offset. The function fitted is of the form

.. math::
  f(t) = N_1 f(t + t_s)_{input} + N_2 f(t)_{input},

where :math:`N_1` is the FirstPulseWeight, :math:`N_2` is the SecondPulseWeight and :math:`t_s` is the PulseOffset. The corresponding parameters of the two input functions are tied to have the same value. In practise this is achieved by taking a convolution of two delta functions with the input function;

.. math::
  f(t) = f(t)_{input} * (N_1\delta(-ts) + N_2\delta(0)).

With the exception of the three new parameters PulseOffset, FirstPulseWeight and SecondPulseWeight the interface for DoublePulseFit is identical to that of Fit and the two can be used interchangeably.
The function output by this algorithm corrospond to the inner function only whilst the output fit curves corrospond to the full function.

Usage
-----

.. code-block:: python

  import numpy as np
  from mantid.simpleapi import DoublePulseFit, CreateWorkspace, GausOsc
  from mantid.api import FunctionFactory

  # Create a workspace with two offset oscillations
  delta = 0.33
  x = np.linspace(0.,15.,100)
  x_offset = np.linspace(delta, 15. + delta, 100)
  testFunction = GausOsc(Frequency = 1.5, A=0.22)
  y1 = testFunction(x)
  y2 = testFunction(x_offset)
  y = y1+y2
  ws = CreateWorkspace(x,y)

  innerFunction = FunctionFactory.createInitialized('name=GausOsc,A=0.2,Sigma=0.2,Frequency=1,Phi=0')
  DoublePulseFit(Function=innerFunction, InputWorkspace=ws,  PulseOffset = delta, StartX=0.0, EndX=15.0, Output='DoublePulseFit')

.. figure:: /images/DoublePulseFitExample.png
  :figwidth: 50%
  :alt: Results of fit compared to initial data.

.. categories::

.. sourcelink::