
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates the elastic signal for the Corelli diffractometer. This is done by calculating the cross-correlation with the correlation chopper. The correlation chopper modulates the incident neutron beam with a pseudo-random sequence. The calculated signal is applied the each event in the form of a weight.

The algorithm requires the timing offset of the TDC signal from the correlation chopper to run. The timing offset is dependant on the frequency of the chopper and should not change if the frequency has not changed.

Usage
-----
..  Try not to use files in your examples, 
    but if you cannot avoid it then the (small) files must be added to 
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CorelliCrossCorrelate**

.. testcode:: CorelliCrossCorrelateExample

   try:
       # Create a host workspace
       ws = CreateSampleWorkspace()
       wsOut = CorelliCrossCorrelate(ws,56000)
   except:
       pass

Output:

.. testoutput:: CorelliCrossCorrelateExample 



.. categories::

