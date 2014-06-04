.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will raise the InputWorkspace to the power of the
Exponent. When acting on an event workspace, the output will be a
Workspace2D, with the default binning from the original workspace.

Errors
------

Defining the power algorithm as: :math:`y = \left ( a^b \right )`, we
can describe the error as: :math:`s_{y} = by\left ( s_{a}/a \right )`,
where :math:`s_{y}` is the error in the result *y* and :math:`s_{a}` is
the error in the input *a*.

.. categories::
