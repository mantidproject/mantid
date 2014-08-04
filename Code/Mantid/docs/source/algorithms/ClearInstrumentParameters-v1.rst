.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm clears all the parameters associated with a workspace's instrument.

Parameters are used by Mantid to tweak an instrument's values without having to change
the `instrument definition file <http://mantidproject.org/InstrumentDefinitionFile>`__ itself.

The LocationParameters property specifies whether or not to clear any calibration parameters
used to adjust the location of any components. Specifically, it will not clear the "x", "y", "z",
"r-position", "t-position", "p-position", "rotx", "roty", and "rotz" parameters.

.. categories::
