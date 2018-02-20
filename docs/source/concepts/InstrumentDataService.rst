.. _Instrument Data Service:

Instrument Data Service
=======================

What is it?
-----------

The Instrument Data Service (IDS) is a :ref:`Data Service <Data Service>`
that is specialized to hold all of the :ref:`instruments <Instrument>` that
are created during a user session. Whenever an instrument definition is
loaded it is saved in the IDS and further workspaces that refer to the
same instrument share the same definition.

How does it work?
-----------------
The Instrument data service is similar to all of the other :ref:`Data Services <Data Service>`
in mantid and is implemented as a simple dictionary object holding keys referring to shared pointers to the base
instrument definitions.  The key is a compound string made up of the Instrument name with a sha1 hash of the text
of the instrument definition appended.  For those detail minded among you, the has is derived specifically by
first converting any lines endings within the definition to linux line endings, and then trimming any white space
from the start and end of the definition before calculating the sha1 checksum.

Extracting an instrument from the Instrument Data Service
---------------------------------------------------------

This is rarely something that a user or an algorithm writer would need
to do as it is all handled by the framework internals. Normally you
would access the instrument relating to a workspace directly though that
workspace.


**Example: Getting the instrument from a workspace**

.. testcode:: GetInstrument

    ws = CreateSampleWorkspace("Event",NumBanks=1,BankPixelWidth=1)
    inst = ws.getInstrument()
    print(inst.getSource().getPos())

Output:

.. testoutput:: GetInstrument

    [0,0,-10]



.. categories:: Concepts
