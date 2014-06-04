.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workspaces contain information in logs. Often these detail what happened
to the sample during the experiment. This algorithm allows one named log
to be entered.

The log can be either a String, a Number, or a Number Series. If you
select Number Series, the workspace start time will be used as the time
of the log entry, and the number in the text used as the (only) value.

If the LogText contains a numeric value, the created log will be of
integer type if an integer is passed and floating point (double)
otherwise. This applies to both the Number & Number Series options.

.. categories::
