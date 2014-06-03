.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm adds or replaces an parameter attached to an instrument
component, or the entire instrument. Instrument parameters are specific
to a workspace, they will get carried on to output workspaces created
from an input workspace to an algorithm, but will not appear one
unrelated workspaces that happen to have been recorded on the same
instrument.

The workspace must have a instrument already defined, and will be
altered in place. If the name of the instrument component to attach the
parameter is not specified it will be attached to the whole instrument.

At present this algorithm only supports simple instrument parameters,
NOT fitting parameters.

.. categories::
