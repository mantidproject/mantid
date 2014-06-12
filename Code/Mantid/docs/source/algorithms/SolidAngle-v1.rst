.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates solid angles from the sample position of the
input workspace for all of the spectra selected. If several detectors
have been mapped to the same spectrum then the solid angles of this
detectors will be summed to provide the solid angle for the spectrum.
The solid angle of a detector that has been masked or marked as dead is
considered to be 0 steradians.

This algorithms can happily accept `ragged
workspaces <http://www.mantidproject.org/Ragged_Workspace>`__ as an input workspace. The result would
be a ragged output workspace whose X axis values match the lowest and
highest of each the input spectra.

Note: The Solid angle calculation assumes that the path between the
sample and detector is unobstructed by another other instrument
components.

.. categories::
