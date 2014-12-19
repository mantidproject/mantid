.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Some sample logs from DAS are written in the format such that the time
stamps are the pulse times and the values are time-of-flight. They are
usually used to record some mono-value sample log such as turning on or
off of a sample environment device. This algorithm will convert sample
logs of this time such that the new log will have the time stamp as the
absolute time, i.e., sum of pulse time and time-of-flight.

This type of DAS sample log won't be generated anymore in NeXus file.  
Hence algorithm ProcessDasNexusLog is deprecated. 

No usage example is needed for a deprecated algorithm. 

.. categories::
