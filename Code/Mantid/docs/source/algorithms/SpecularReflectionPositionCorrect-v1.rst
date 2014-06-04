.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Uses the specular reflection condition along with a supplied theta value
to vertically shift the detectors into a corrected location.

ThetaIn == ThetaOut

and

:math:`2*ThetaOut = tan^{-1}\frac{UpOffset}{BeamOffset}`

For LineDetectors and MultiDetectors, the algorithm uses an average of
grouped detector locations to determine the detector position.

.. categories::
