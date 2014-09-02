.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a workspace and a value for two theta, and attempts to calculate
the reflectometry resolution (dq/q) from them. If no value is provided for two theta
then CalculateResolution will attempt to fetch a value from the workspace's log
using the two theta log name provided.

CalculateResolution outputs two values, the calculated resolution and the value of
TwoTheta that was used in the calculation. The latter is useful when TwoTheta was not
given to CalculateResolution, causing CalculateResolution to extract it from the
workspace's sample log.

Usage
-----

.. testcode::

  ws = Load('INTER00013460')
  res, two_theta = CalculateResolution(Workspace = ws, TwoTheta = 0.7)
  print("Resolution: %.4f" % res)
  print("Two Theta: %.4f" % two_theta)

.. testoutput::

  Resolution: 0.0340
  Two Theta: 0.7000

.. categories::
