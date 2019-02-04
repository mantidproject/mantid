.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Workflow algorithm to determine chunking strategy
for event nexus file. This will break up the instrument
into logical components according to the instrument hierarchy.

This algorithm does assume that there are components in the
geometry named "bank#" and returns a table workspace of those
names. Trying this on an instrument without components named
that will generate an exception. Also, requesting "ChunkBy=All"
will return an empty table workspace.

Usage
-----

**Example: Powgen**

.. testcode:: ExPowgen

   pg3 = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
   ws = CreateChunkingFromInstrument(InputWorkspace=pg3, ChunkBy="Group")
   print("Created {} Chunks".format(ws.rowCount()))

Output:

.. testoutput:: ExPowgen

   Created 4 Chunks

**Example: Snap**

.. testcode:: ExSnap

   ws = CreateChunkingFromInstrument(InstrumentName="snap", ChunkNames="East,West", MaxBankNumber=64)
   print("Created {} Chunks".format(ws.rowCount()))

Output:

.. testoutput:: ExSnap

   Created 2 Chunks

.. categories::

.. sourcelink::
