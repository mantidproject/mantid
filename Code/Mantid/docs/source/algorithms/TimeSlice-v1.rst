.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Used to perform integration over a given time of flight data from an
indirect inelastic geometry instrument.

The peak range will be integrated to give the result over the given
spectra range, the output workspace will contain the result of the
integration against the spectrum number.

Optionally a calibration workspace can be provided which will be
applied to the raw data before the integration is performed.

A background range can also be provided which will first calculate
and subtract a flat background from the raw data before the
integration is performed.

.. categories::
