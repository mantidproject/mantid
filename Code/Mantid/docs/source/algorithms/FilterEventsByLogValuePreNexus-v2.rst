.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadEventPreNeXus algorithm stores data from the pre-nexus neutron
event data file in an :ref:`EventWorkspace <EventWorkspace>`. The default
histogram bin boundaries consist of a single bin able to hold all events
(in all pixels), and will have their `units <http://www.mantidproject.org/units>`_ set to
time-of-flight. Since it is an :ref:`EventWorkspace <EventWorkspace>`, it
can be rebinned to finer bins with no loss of data.

Optional properties
###################

Specific pulse ID and mapping files can be specified if needed; these
are guessed at automatically from the neutron filename, if not
specified.

A specific list of pixel ids can be specified, in which case only events
relating to these pixels will appear in the output.

The ChunkNumber and TotalChunks properties can be used to load only a
section of the file; e.g. if these are 1 and 10 respectively only the
first 10% of the events will be loaded.

.. categories::
