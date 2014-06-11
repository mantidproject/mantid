.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Corrects the flight paths of a flat detector.

Both TOF sample-detector and distance sample-detector are corrected to constant values, i.e., this algorithm make the detector spherical rather than flat.

The sample to detector distance must be specified as **l2** in the instrument parameters file.

So far this has only be tested on the ILL IN5.

Note
###################################
This algorithm was coded as a proof of concept. It may be deprecated in the future.


.. categories::
