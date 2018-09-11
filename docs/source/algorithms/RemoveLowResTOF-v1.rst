.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The low resolution time-of-flight cutoff is determined by one of two methods. 
The selection is made based on whether or not ``MinWavelength`` is specified.

MinWavelength
#############

If the minimum wavelength is specified, then the mimimum time-of-flight for each pixel is calculated by converting the ``MinWavelength`` to time-of-flight using the standard equation found in :ref:`algm-ConvertUnits`.

Hodges Criteria
###############

First is calculated the value of :math:`dspmap = 1/DIFC`. Then the value of

:math:`sqrtdmin = \sqrt{T_{min} / DIFC_{ref}} + m_K * \log_{10}(dspmap * DIFC_{ref})`

If this is a negative number then the minimum time-of-flight is set to zero. Otherwise
it is caculated as

:math:`tmin = sqrtdmin * sqrtdmin / dspmap`

.. categories::

.. sourcelink::
