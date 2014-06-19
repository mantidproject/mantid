.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The LoadEventPreNeXus algorithm stores data from the pre-nexus neutron
event data file in an `EventWorkspace <EventWorkspace>`__. The default
histogram bin boundaries consist of a single bin able to hold all events
(in all pixels), and will have their `units <units>`__ set to
time-of-flight. Since it is an `EventWorkspace <EventWorkspace>`__, it
can be rebinned to finer bins with no loss of data.

Optional properties
###################

Specific pulse ID and mapping files can be specified if needed; these
are guessed at automatically from the neutron filename, if not
specified.

.. categories::
