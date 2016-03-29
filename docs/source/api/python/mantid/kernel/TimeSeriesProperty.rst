.. _TimeSeriesProperty:

==================
TimeSeriesProperty
==================

A TimeSeriesProperty is a specialised :class:`mantid.kernel.Property`
class that holds time/value pairs. It offers a selection of statistics
through it's Python interface which you can use in your scripts.

Introduction
============

To get hold of a time series property, you need to get the handle to the object from the :class:`mantid.api.Run` object.

To get the :class:`mantid.kernel.TimeSeriesPropertyStatistics` object,
you then call ``getStatistics()`` on the property.

This allows you to access the following attributes:

* minimum
* maximum
* mean
* median
* standard_deviation
* duration

Functions
=========

``filterWith()``
----------------

**Handling boundary condition**

Definition: ``log_t0``, ``log_tf``, ``filter_t0``, ``filter_tf``

* Beginning of the filter

  * If ``filter_t0`` < `log_t0``, then the log is extended to ``filter_t0``

  * If ``filter_t0`` > ``log_t0``, all logs before first occurrence of False in filter are in the prohibited region.

It is to say that the first entry of a log starts from the first occurrence of TRUE value.

* End of the filter

  * If ``filter_tf`` > ``log_tf``, and ``filter_tf`` is false, the (virtual) filtered log is extended by all filter entries beyond ``log_tf``;

  * If ``filter_tf`` < `log_tf``, and last filter entry is false, then all entries of the log after ``filter_tf`` are in the disallowed region;

``nthInterval(n)``
------------------

Return the nth interval

* An interval starts from filter's time.

* If the starting filter time is not same as any log entry, then from this filter time to the log entry just behind it will be an interval.

* An interval ends at filter's time if there is a filter value change between this log entry and its next log entry;

* An interval can go beyond real log.

* If it is the last interval, dt is estimated from either previous log entry or previous (false) filter entry, which is later in time.

``nthValue(n)``
---------------

Return the value of nth interval.

* If the interval starts from a filter time, then the value is either

  * the log value just before the filter time if filter time is not ahead all log entries' time or
  * the value of first log entry
