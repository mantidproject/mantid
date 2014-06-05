.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm integrates up the instrument hierarchy, and each pixel
will contain the average value for the component. For example, assuming
that for a particular instrument on workspace w1 a "tube" is made out of
"pixels", w=IntegrateByComponent(w1,1) will integrate values of w1,
calculate the average along the tube (LevelsUp=1) (for non-masked
pixels), and replace the value of each spectrum in a tube with the
average value for that tube.

Note that if the detectors are grouped before, this algorithm won't run
except with LevelsUp=0 (integrate over all detectors).

.. categories::
