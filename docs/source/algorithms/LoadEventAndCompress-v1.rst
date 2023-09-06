
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that loads an event nexus file in chunks
and compresses the resulting chunks before summing them. It uses the
algorithms:

#. :ref:`algm-DetermineChunking`
#. :ref:`algm-LoadEventNexus`
#. :ref:`algm-FilterBadPulses`
#. :ref:`algm-CompressEvents`
#. :ref:`algm-Plus` to accumulate


Workflow
########

.. diagram:: LoadEventAndCompress-v1_wkflw.dot


Usage
-----
**Example - LoadEventAndCompress**

The files needed for this example are not present in our standard usage data
download due to their size.  They can however be downloaded using these links:
`PG3_9830_event.nxs <https://github.com/mantidproject/systemtests/blob/master/Data/PG3_9830_event.nxs?raw=true>`_.


.. code-block:: python

   PG3_9830_event = LoadEventAndCompress(Filename='PG3_9830_event.nxs',
                                         MaxChunkSize=1.)

.. categories::

.. sourcelink::
