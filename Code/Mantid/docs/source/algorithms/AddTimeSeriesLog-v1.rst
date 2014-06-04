.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates/updates a time-series log entry on a chosen workspace. The given
timestamp & value are appended to the named log entry. If the named
entry does not exist then a new log is created. A time stamp must be
given in ISO8601 format, e.g. 2010-09-14T04:20:12.

By default, the given value is interpreted as a double and a double
series is either created or expected. However, if the "Type" is set to
"int" then the value is interpreted as an integer and an integer is
either created or expected.

.. categories::
