.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Compares two workspaces for equality. This algorithm is mainly intended
for use by Mantid developers as part of the testing process.

The data values (X,Y and error) are always checked. The algorithm can
also optionally check the axes (this includes the units), the
spectra-detector map, the instrument (the name and parameter map) and
any bin masking.

In the case of `EventWorkspaces <EventWorkspace>`__, they are checked to
hold identical event lists. Comparisons between an EventList and a
Workspace2D always fail.

.. categories::
