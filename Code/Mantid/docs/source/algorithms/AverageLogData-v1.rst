.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm will calculate a proton\_charge weighted average and
standard deviation of any log value of numeric series type. All proton
charges earlier than the first data are ignored. Any proton pulse is
counted for the log value on the right. This means that if all proton
pulses happen before the first value, and FixZero is false, the average
and standard deviations are NANs. If all the proton pulses occur after
the last value, and FixZero is false, the average is equal to the last
value, and the standard deviation is zero.

.. categories::
