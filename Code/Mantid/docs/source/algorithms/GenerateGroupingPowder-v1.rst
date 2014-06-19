.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

For powder samples, with no texture, the scattering consists only of
rings. This algorithm reads a workspace and an angle step, then
generates a grouping file (.xml) and a par file (.par), by grouping
detectors in intervals i\*step to (i+1)\*step. The par file is required
for saving in the NXSPE format, since Mantid does not correctly
calculates the correct angles for detector groups. It will contain
average distances to the detector groups, and average scattering angles.
The x and y extents in the par file are radians(step)\*distance and
0.01, and are not supposed to be accurate.

.. categories::
