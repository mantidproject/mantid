.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm allows instrument parameters to be saved into an
`instrument parameter file <http://mantidproject.org/InstrumentParameterFile>`__.
The parameter file can then be inspected and or modified. It can also be loaded back into
Mantid using the `LoadParameterFile <http://mantidproject.org/LoadParameterFile>`__ algorithm.

The LocationParameters property specifies whether or not to save any calibration parameters
used to adjust the location of any components. Specifically, it will skip "x", "y", "z",
"r-position", "t-position", "p-position", "rotx", "roty", and "rotz" parameters.
