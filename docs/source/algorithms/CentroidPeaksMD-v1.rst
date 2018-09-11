.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm starts with a PeaksWorkspace containing the expected
positions of peaks in reciprocal space. It calculates the centroid of
the peak by calculating the average of the coordinates of all events
within a given radius of the peak, weighted by the weight (signal) of
the event.

The V1 of the algorithm is deprecated and left for compartibility with the scripts, 
which have the property  *CoordinatesToUse* set. 
Use :ref:`algm-CentroidPeaksMD-v2` for any new scripts.


.. categories::

.. sourcelink::
