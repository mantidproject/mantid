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

.. categories::
