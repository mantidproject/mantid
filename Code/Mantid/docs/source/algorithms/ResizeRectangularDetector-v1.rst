.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will resize a
`RectangularDetector <RectangularDetector>`__ by applying X and Y
scaling factors. Each pixel's position will be modifed relative to the
0,0 point of the detector by these factors. Typically, a
RectangularDetector is constructed around its center, so this would
scale the detector around its center.

This only works on `RectangularDetectors <RectangularDetector>`__. Banks
formed by e.g. tubes cannot be scaled in this way.

Internally, this sets the "scalex" and "scaley" parameters on the
`RectangularDetector <RectangularDetector>`__. Note that the scaling is
relative to the original size, and is not cumulative: that is, if you
Resize \* 2 and again \* 3, your final detector is 3 times larger than
the original, not 6 times.

Note: As of this writing, the algorithm does NOT modify the shape of
individual pixels. This means that algorithms based on solid angle
calculations might be off. Ray-tracing (e.g. peak finding) are
unaffected.

See also :ref:`algm-MoveInstrumentComponent` and
:ref:`algm-RotateInstrumentComponent` for other ways
to move components.

.. categories::
