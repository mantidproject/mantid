.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Transfer an instrument parameters from a giving workspace to a receiving
workspace.

The instrument parameters in the receiving workspace are REPLACED
(despite you can assume from the name of the algorithm) by a copy of the
instrument parameters in the giving workspace so gaining any
manipulations such as calibration done to the instrument in the giving
workspace.

Does not work on workspaces with grouped detectors if some of the
detectors were calibrated.

.. categories::
