.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Attempts to load information about the instrument from a ISIS NeXus
file. In particular attempt to read L2 and 2-theta detector position
values and add detectors which are positioned relative to the sample in
spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also adds dummy
source and samplepos components to instrument.

LoadInstrumentFromNexus is intended to be used as a child algorithm of
other Load algorithms, rather than being used directly. It is used by
LoadMuonNexus version 1.

Usage
-----

**Example - Loading Instrument from Nexus**

.. include:: ../usagedata-note.txt

.. testcode:: Ex

   # Create a dummy ws containing arbitrary data, into which we will load the instrument.
   ws = CreateSampleWorkspace("Histogram","Flat background")
   LoadInstrumentFromNexus(ws, "MUSR00015189.nxs")
   inst = ws.getInstrument()

   print "The name of the instrument is \"%s\"." % inst.getName().strip()
   print ("The reference frame axis pointing along the beam is %s." % 
         inst.getReferenceFrame().pointingAlongBeam())

Output:

.. testoutput:: Ex

   The name of the instrument is "MUSR".
   The reference frame axis pointing along the beam is Z.

.. categories::
