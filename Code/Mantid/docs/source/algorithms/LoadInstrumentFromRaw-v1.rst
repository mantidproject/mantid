.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Attempts to load information about the instrument from a ISIS raw file. In
particular, attempts to  read L2 and 2-theta detector position values and
add detectors which are positioned relative to the sample in spherical
coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also adds dummy source and
samplepos components to instrument.

LoadInstrumentFromRaw is intended to be used as a child algorithm of 
other Load algorithms, rather than being used directly.    

Usage
-----

**Example - Saving Some Pre-existing Data**

.. include:: ../usagedata-note.txt

.. testcode:: Ex

   # Create a dummy ws containing arbitrary data, into which we will load the instrument.
   ws = CreateSampleWorkspace("Histogram","Flat background")

   det_id_list = LoadInstrumentFromRaw(ws, "LOQ48127.raw")

   inst = ws.getInstrument()

   print "The name of the instrument is \"%s\"." % inst.getName().strip()
   print "The position of the source is %s." % str(inst.getSource().getPos())
   print "The position of detector 5 is %s." % str(inst.getDetector(5).getPos())
   print "Is detector 1 a monitor? " + str(inst.getDetector(1).isMonitor())
   print "Is detector 8 a monitor? " + str(inst.getDetector(8).isMonitor())

Output:

.. testoutput:: Ex

   The name of the instrument is "LOQ".
   The position of the source is [0,0,-11].
   The position of detector 5 is [0,0,-11.15].
   Is detector 1 a monitor? True
   Is detector 8 a monitor? False

.. categories::
