.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Filters out logs that do not sit between StartTime and EndTime. The
algorithm also applied a 'Method' to those filtered results and returns
the statistic. A workspace must be provided containing logs. The log
name provided must refer to a FloatTimeSeries log.

Unless specified, StartTime is taken to be run\_start. StartTime and
EndTime filtering is inclusive of the limits provided.

The Method allows you to create quick statistics on the filtered array
returned in the FilteredResult output argument. Therefore the return
value from Method=mean is equivalent to running numpy.mean on the output
from the FilteredResult property. All the Method options map directly to
python numpy functions with the same name. These are documented
`here <http://docs.scipy.org/doc/numpy/reference/routines.statistics.html>`__

.. categories::
