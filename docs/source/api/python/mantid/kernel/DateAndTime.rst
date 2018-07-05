=============
 DateAndTime
=============

This a python binding to the C++ class Mantid::Kernel::DateAndTime.

The equivalent object in python is :class:`numpy.datetime64`. The two
classes have a different EPOCH. Note that
:class:`mantid.kernel.DateAndTime` uses the GPS epoch of 1990, where
:class:`numpy.datetime64` uses the unix epoch of 1970.  For that
reason, there is an additional method
:meth:`mantid.kernel.DateAndTime.to_datetime64`.

To convert an array of :class:`mantid.kernel.DateAndTime`, analgous to
what :meth:`mantid.kernel.FloatTimeSeriesProperty.times` does
internally, use the code:

.. code-block:: python

   times = np.array(times, dtype=np.int64) * np.timedelta64(1,'ns') + np.datetime64('1990-01-01T00:00')

With :class:`numpy.datetime64`, finding the number of seconds between two times is simply

.. code-block:: python

   diff = (timeArray[-1]-timeArray[0]) / np.timedelta64(1, 's')

For converting :class:`numpy.datetime64` data to a string for the default facilty, use :ref:`numpy.datetime_as_string` explicitly (assuming the times are a variable named ``times``)

.. code-block:: python

   import pytz
   tz = pytz.timezone(ConfigService.getFacility().timezone())
   times = numpy.datetime_as_string(times, timezone=tz)

This is how numpy internally converts :class:`numpy.datetime64` to strings for printing, but the selection of timezone is explicit (using `pytz <https://pythonhosted.org/pytz/>`_) to allow for matching where the data was recorded rather than the system's timezone.

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.DateAndTime
    :members:
    :undoc-members:
    :inherited-members:
