- :class:`~mantid.geometry.DetectorInfo`
  and :class:`~mantid.api.SpectrumInfo` in Python have new methods ``phi`` and ``phiOffset``,
  and new versions of ``twoTheta`` and ``signedTwoTheta`` that take an observer position and axis.
  These give the same results as ``getPhi``, ``getPhiOffset``, ``getTwoTheta`` and ``getSignedTwoTheta``
  on detector objects, so they can be used in their place.
