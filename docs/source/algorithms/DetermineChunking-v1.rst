.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Workflow algorithm to determine chunking strategy for event nexus,
raw, or histo nexus files

Usage
-----

**Example**

.. testcode:: ExDeterminChuncking

    ws=DetermineChunking("CNCS_7860_event.nxs",MaxChunkSize=0.0005)
    print("A max chunk size of 0.0005 created {} chunks.".format(ws.rowCount()))


Output:

.. testoutput:: ExDeterminChuncking

    A max chunk size of 0.0005 created 11 chunks.

.. categories::

.. sourcelink::
