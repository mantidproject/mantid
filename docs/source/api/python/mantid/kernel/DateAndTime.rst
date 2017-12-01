=============
 DateAndTime
=============

This a python binding to the C++ class Mantid::Kernel::DateAndTime.

The equivalent object in python is :class:`numpy.datetime64`. The two
classes have a different EPOCH. For that reason there are currently
two suggested ways to convert :class:`mantid.kernel.DateAndTime` into
a :class:`numpy.datetime64`.

.. code-block:: python

   time = np.datetime64(str(time).strip()) # convert to array of strings
   time = np.timedelta64(a.total_nanoseconds(), 'ns') + np.datetime64('1990-01-01T00:00')

Note that :class:`mantid.kernel.DateAndTime` uses the GPS epoch of 1990,
where :class:`numpy.datetime64` uses the unix epoch of 1970.

To convert an array of :class:`mantid.kernel.DateAndTime`, such as in
:meth:`mantid.kernel.FloatTimeSeriesProperty.times`, use the code:

.. code-block:: python

   pcharge = wksp.run()['proton_charge']
   times = np.array(pcharge.times, dtype=np.int) * np.timedelta64(1,'ns') + np.datetime64('1990-01-01T00:00')

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.DateAndTime
    :members:
    :undoc-members:
    :inherited-members:
