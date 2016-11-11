.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm will be deprecated in the next version of Mantid. Please, use :ref:`algm-ConvertToConstantL2` instead, which
   is the new name for this algorithm.

Moves the instrument and then corrects the flight paths such that a flat detector appears spherical with a constant l2 value.

Both time-of-flight sample-detector time and sample to detector distance are corrected to constant values.

The sample to detector distance must be specified as **l2** in the instrument parameters file.

So far this has only be tested on the ILL IN5 instrument.

Note
###################################
This algorithm is intended for visualisaion only. It is not recommended as part of any reduction process.


.. categories::

.. sourcelink::
