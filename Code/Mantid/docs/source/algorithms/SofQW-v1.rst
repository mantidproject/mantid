.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is for use by inelastic instruments and takes as its
input a workspace where the data's been reduced to be in units of energy
transfer against spectrum number (which can be seen as equivalent to
angle, with the angle being taken from the detector(s) to which the
spectrum pertains). For each bin the value of momentum transfer
(:math:`q`) is calculated, and the counts for that bin are assigned to
the appropriate :math:`q` bin.

The energy binning will not be changed by this algorithm, so the input
workspace should already have the desired bins (though this axis can be
rebinned afterwards if desired). The EMode and EFixed parameters are
required for the calculation of :math:`q`.

If the input workspace is a distribution (i.e. counts / meV ) then the
output workspace will similarly be divided by the bin width in both
directions (i.e. will contain counts / meV / (1/Angstrom) ).

.. categories::
