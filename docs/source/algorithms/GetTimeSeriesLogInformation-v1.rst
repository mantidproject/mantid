.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Get information from a TimeSeriesProperty log.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - Get Information Using Run Object (Recommended)**

.. testcode:: exGetTimeSeriesLogInformationRunObject

   w=Load('CNCS_7860')

   speed5_stats = w.run().getStatistics("Speed5")
   print("duration {:.3f}".format(speed5_stats.duration))
   print("minimum {:.3f}".format(speed5_stats.minimum))
   print("maximum {:.3f}".format(speed5_stats.maximum))
   print("mean {:.3f}".format(speed5_stats.mean))
   print("time_mean {:.3f}".format(speed5_stats.time_mean))

Output:

.. testoutput:: exGetTimeSeriesLogInformationRunObject

   duration 171.702
   minimum 299.800
   maximum 300.000
   mean 299.900
   time_mean 299.888

**Example - Get Information from One Log**

.. testcode:: exGetTimeSeriesLogInformationSimple

   w=Load('CNCS_7860')

   result = GetTimeSeriesLogInformation(InputWorkspace=w,LogName='Speed5',InformationWorkspace='info')

   for i in [0,5,6,7,8]:
      row = result[1].row(i)
      print("{Name}  {Value:.3f}".format(**row))

Output:

.. testoutput:: exGetTimeSeriesLogInformationSimple

   Items  4.000
   Average(dT)  39.239
   Max(dT)  53.984
   Min(dT)  29.953
   Sigma(dt)  10.543

.. categories::

.. sourcelink::
