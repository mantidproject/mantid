.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a workspaces and a value for theta, and attempts to calculate
the reflectometry resolution (dq/q) from them. If no value is provided for theta
then CalculateResolution will attempt to fetch a value from the workspace's log.

This algorithm depends upon proper configuration of reflectometry instruments
passed to it. Specifically, it requires that two components, 'slit1', and 'slit2'
are properly defined and positioned, and that they both have their 'vertical gap'
parameter set correctly. If these requirements are not met, this algorithm will
not produce accurate results.

Usage
-----

.. testcode::

  ws = Load('INTER00013460')
  res = CalculateResolution(Workspace = ws, Theta = 0.7)
  print("Resolution: %.4f" % res)

.. testoutput::

  Resolution: 0.0340

.. categories::
