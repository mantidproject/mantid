.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The diagnostic test attempts to find all tubes within the instrument
attached to the workspace. If successful, each tube is tested for
saturation above the level defined by the 'MaxTubeFramerate' property.
If any pixel, not including those marked to be ignored around the
equatorial region, are counting above this threshold then the entire
tube is masked.

Restrictions on the input workspace
###################################

-  The workspace must contain either raw counts or counts/us.

.. categories::
