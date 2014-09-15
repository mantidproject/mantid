.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Get information from a TimeSeriesProperty log.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - Get Information from One Log**

.. testcode:: exGetTimeSeriesLogInformationSimple

   w=Load('CNCS_7860')

   result = GetTimeSeriesLogInformation(InputWorkspace=w,LogName='Speed5',InformationWorkspace='info')

   for i in [0,5,6,7,8]:
      row = result[1].row(i)
      print row['Name'],  " %.3f" % row['Value']

Output:

.. testoutput:: exGetTimeSeriesLogInformationSimple

   Items  4.000
   Average(dT)  39.239
   Max(dT)  53.984
   Min(dT)  29.953
   Sigma(dt)  10.543
   
.. categories::
