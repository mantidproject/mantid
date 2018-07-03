.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Subtract the dark current from an EQSANS data set. 
This algorithm is rarely called directly. It is called by 
:ref:`SANSReduction <algm-SANSReduction>`.

This workflow algorithm will:

- Properly load the dark current data set

- Normalize the dark current to the data taking period

- Subtract the dark current from the input workspace


The dark current is subtracted pixel by pixel by normalizing the dark current data by counting time. The total number of dark current counts :math:`N_{dc}(i)` for each pixel i is obtained by integrating over all time of flight bins. For a given pixel and wavelength bin, the corrected signal is given by:

:math:`I'(i,\lambda_j)=I_{data}(i,\lambda_j)-N_{dc}(i) \ \ \frac{T_{data}}{T_{dc}} \ \ \frac{t_{frame}-t^{low}_{cut} - t^{high}_{cut}}{t_{frame}} \ \ \frac{\Delta\lambda_j}{\lambda_{max}-\lambda_{min}}`

where the T-values are the counting times for the data set and the dark current (dc). The :math:`t_{cut}` values are the TOF cuts at the beginning and end of a frame. :math:`t_{frame}` is the width of a frame. :math:`\Delta\lambda_j` is the width of the wavelength bin we are considering. 

.. categories::

.. sourcelink::
