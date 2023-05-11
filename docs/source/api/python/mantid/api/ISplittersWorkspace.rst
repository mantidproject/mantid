.. _SplittersWorkspace:

===================
Splitters Workspace
===================

SplittersWorkspaces stores a vector of SplittingIntervals. It
inherits from :ref:`TableWorkspace <Table Workspaces>`.

A SplittersWorkspace object has 3 columns, ``start`` with type as
``int64``, ``stop`` with type as ``int64`` and ``workspacegroup`` as
``int``.  Column ``start`` is to record the starting absolute time
of a :ref:`SplittingInterval`. Column ``stop`` is to record the stopping
absolute time of a :ref:`SplittingInterval`. Both starting and stopping
time must be in nanoseconds and be relative to :class:`GPS epoch <mantid.kernel.DateAndTime>`
Column ``workspacegroup`` is to record the index of a workspace which will get
the events that fall into the time interval defined by ``start`` and ``stop``.


.. _SplittingInterval:

Splitting Interval
------------------

SplittingInterval is a class to store and process the event splitting
information. It stores and returns the starting and stopping time of a
time-splitter as well as an 'index'. The 'index' (0-based) denotes which
workspace will get the events that fall into the [start,stop) time interval.
An 'index' of -1 means that events in a given time interval will be "unfiltered".

.. autoclass:: mantid.api.ISplittersWorkspace
    :members:
    :undoc-members:
    :inherited-members:
