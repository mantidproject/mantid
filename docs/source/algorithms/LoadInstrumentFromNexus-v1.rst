.. algorithm::

.. summary::

.. relatedalgorithms::

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
   compInfo = ws.componentInfo()

   print("The name of the instrument is '{}'.".format(compInfo.name(compInfo.root()).strip()))
   print("The source position is at:  {}.".format(compInfo.sourcePosition()))


Output:

.. testoutput:: ExLoadMUSR

   The name of the instrument is 'MUSR'.
   The source position is at:  [0,-10,0].

.. categories::

.. sourcelink::
