=============
 DateAndTime
=============

This is a Python binding to the C++ class Mantid::Kernel::DateAndTime.

The equivalent object in python is :class:`numpy.datetime64`. The two
classes have a different EPOCH. Note that
:class:`mantid.kernel.DateAndTime` uses the GPS epoch of 1990, where
:class:`numpy.datetime64` uses the unix epoch of 1970.  For that
reason, there is an additional method
:meth:`mantid.kernel.DateAndTime.to_datetime64`.

To convert an array of :class:`mantid.kernel.DateAndTime`, analogous to
what :meth:`mantid.kernel.FloatTimeSeriesProperty.times` does
internally, use the code:

.. code-block:: python

   times = np.array(times, dtype=np.int64) * np.timedelta64(1,'ns') + np.datetime64('1990-01-01T00:00')

With :class:`numpy.datetime64`, finding the number of seconds between two times is simply

.. code-block:: python

   diff = (timeArray[-1]-timeArray[0]) / np.timedelta64(1, 's')

For converting `numpy.datetime64 <https://docs.scipy.org/doc/numpy/reference/arrays.datetime.html>`_ data to a string for the default facilty, use :func:`numpy.datetime_as_string` explicitly (assuming the times are a variable named ``times``)

.. code-block:: python

   import pytz
   tz = pytz.timezone(ConfigService.getFacility().timezone())
   times = numpy.datetime_as_string(times, timezone=tz)

This is how numpy internally converts `numpy.datetime64 <https://docs.scipy.org/doc/numpy/reference/arrays.datetime.html>`_ to strings for printing, but the selection of timezone is explicit (using `pytz <https://pythonhosted.org/pytz/>`_) to allow for matching where the data was recorded rather than the system's timezone.

Converting strings to ``datetime64``
------------------------------------

How strings are interpreted by `numpy.datetime64 <https://docs.scipy.org/doc/numpy/reference/arrays.datetime.html>`_  `changed in version 1.11 <https://docs.scipy.org/doc/numpy/reference/arrays.datetime.html#changes-with-numpy-1-11>`_. Before this point, a string without timezone designation was assumed to be in local timezone. After this point it is assumed to be in UTC. Additionally as of version 1.11, NumPy started warning if a string was supplied with timezone designation. This means that on rhel7 (in EDT)

.. code-block:: python

   repr(np.datetime64('1990-01-01T00:00Z'))
   repr(np.datetime64('1990-01-01T00:00'))

result in ``"numpy.datetime64('1989-12-31T19:00-0500')"`` and ``"numpy.datetime64('1990-01-01T00:00-0500')"`` respectively. However, in "newer" python results ``"numpy.datetime64('1990-01-01T00:00')"``, but the first version results in a ``DeprecationWarning``.

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.DateAndTime
    :members:
    :undoc-members:
    :inherited-members:
