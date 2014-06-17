.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm starts with a PeaksWorkspace containing the expected
positions of peaks in detector space. It calculates the centroid of the
peak by calculating the average of the coordinates of all events within
a given radius of the peak, weighted by the weight (signal) of the event
for event workspaces or the intensity for histogrammed workspaces.

.. categories::
