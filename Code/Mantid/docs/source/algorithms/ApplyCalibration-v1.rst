.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Update detector positions from input table workspace. The positions are
updated as absolute positions and so this update can be repeated.

The PositionTable must have columns *Detector ID* and *Detector
Position*. The entries of the *Detector ID* column are integer referring
to the Detector ID and the enties of the *Detector Position* are
`V3Ds <V3D>`__ referring to the position of the detector whose ID is in
same row.

.. categories::
