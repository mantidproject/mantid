.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Loads an existing file of pre-calculated or measured absorption
coefficients for the PEARL instrument.

If the file contains "t=" on the first line then the number following
this is assumed to be the thickness in mm. The values in the second
column are assumed to be :math:`\alpha(t)`. Upon reading the file the
:math:`\alpha` values for transformed into attenuation coefficients via
:math:`\frac{I}{I_0} = exp(-\alpha * t)`.

If the file does not contain "t=" on the top line then the values are
assumed to be calculated :math:`\frac{I}{I_0}` values and are simply
read in verbatim.

.. categories::

.. sourcelink::
