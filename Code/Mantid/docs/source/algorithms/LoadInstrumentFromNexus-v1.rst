.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Attempts to load information about the instrument from a ISIS NeXus file. 
In particular, it attempts to read the reference frame axis. 
Also adds dummy source and samplepos components to instrument.

LoadInstrumentFromNexus is intended to be used as a child algorithm of
other Load algorithms, rather than being used directly. 

Usage
-----

**Example - Loading Instrument from Nexus**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadMUSR

   # Create a dummy ws containing arbitrary data, into which we will load the instrument.
   ws = CreateSampleWorkspace("Histogram","Flat background")
   LoadInstrumentFromNexus(ws, "MUSR00015189.nxs")
   inst = ws.getInstrument()
   source = inst.getSource()

   print "The name of the instrument is \"%s\"." % inst.getName().strip()
   print ("The source postion is at:  %s." % str(source.getPos()) )


Output:

.. testoutput:: ExLoadMUSR

   The name of the instrument is "MUSR".
   The source postion is at:  [0,-10,0].

.. categories::
