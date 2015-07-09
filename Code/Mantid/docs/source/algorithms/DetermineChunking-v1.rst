.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Workflow algorithm to determine chunking strategy for event nexus,
runinfo.xml, raw, or histo nexus files

Usage
-----

**Example**

.. testcode:: ExDeterminChuncking

    ws=DetermineChunking("CNCS_7860_event.nxs",MaxChunkSize=0.0005)
    print "A max chunck size of 0.0005 created %i chunks." % ws.rowCount()

    #The algorithm can also use the SNS runinfo.xml file
    ws2=DetermineChunking("CNCS_7860_runinfo.xml",MaxChunkSize=0.0010)
    print "A max chunck size of 0.0010 created %i chunks." % ws2.rowCount()


Output:

.. testoutput:: ExDeterminChuncking

    A max chunck size of 0.0005 created 11 chunks.
    A max chunck size of 0.0010 created 6 chunks.

.. categories::

.. sourcelink::
