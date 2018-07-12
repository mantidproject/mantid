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


The dark current is subtracted pixel by pixel by normalizing the dark current data by counting time, as follows:

:math:`I'(x,y)=I_{data}(x,y)-\frac{T_{data}}{T_{dc}} I_{dc}(x,y)`

where the T-values are the counting times for the data set and the dark current (dc).

The example above uses the counting time for normalization. If the proton charge is
available, the normalization will be done using the proton charge instead.

.. categories::

.. sourcelink::
