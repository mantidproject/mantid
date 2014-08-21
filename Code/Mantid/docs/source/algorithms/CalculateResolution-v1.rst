.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a workspace and a value for theta, and attempts to calculate
the reflectometry resolution (dq/q) from them. If no value is provided for theta
then CalculateResolution will attempt to fetch a value from the workspace's log
using the theta log name provided.

Usage
-----

.. testcode::

  ws = Load('INTER00013460')
  res = CalculateResolution(Workspace = ws, Theta = 0.7)
  print("Resolution: %.4f" % res)

.. testoutput::

  Resolution: 0.0340

.. categories::
