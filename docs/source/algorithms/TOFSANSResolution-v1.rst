.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Compute the Q resolution for a given I(Q) for a TOF SANS instrument. The Q resolution is given as:

    :math:`\left( \frac{dQ}{Q}\right)^2 = \left( \frac{d\theta}{\theta} \right)^2 + \left( \frac{dt}{t} \right)^2 + \left( \frac{\Delta\lambda}{\lambda} \right)^2`

where :math:`t` is the time of flight, :math:`\Delta\lambda` is the wavelength bin width used when histogramming the data, and

    :math:`\left( \frac{d\theta}{\theta}\right)^2 = \frac{1}{12\theta^2}\left[ \frac{3 \ R_1^2}{L_1^2} + \frac{3 \ R^2_2 D^2}{L_1^2 L^2_2} + \frac{2 \ (p_x^2 + p_y^2)}{L^2_2} \right]`

.. categories::

.. sourcelink::
