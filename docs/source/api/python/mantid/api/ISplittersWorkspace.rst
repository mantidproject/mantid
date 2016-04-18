.. _SplittersWorkspace:

===================
Splitters Workspace
===================

SplittersWorkspaces stores a vector of SplittingIntervals. It
inherits from :ref:`TableWorkspace <Table Workspaces>`.

A SplittersWorkspace object has 3 columns, ``start`` with type as
``int64``, ``stop`` with type as ``int64`` and ``workspacegroup`` as
``int``.  Column ``start`` is to record the starting absolute time
(DateTime reference!) of a TimeSplitter.  Column ``stop`` is to record
the stopping absolute time of a :ref:`SplittingInterval`.  Column
``workspacegroup`` is to record what workspace should the events fall
into the time period defined by ``start`` and ``stop``.

.. _SplittingInterval:

Splitting Interval
------------------

SplittingInterval is a class to store and process the event splitting
information. It stores and returns the starting and stopping time of a
time-splitter. Each SplitterInterval will have an 'index'. The 'index'
(starting from zero) denotes which workspace to which any neutron
event falls into its time interval will be copied. An 'index' of -1 to
not filter the events.

.. autoclass:: mantid.api.ISplittersWorkspace
    :members:
    :undoc-members:
    :inherited-members:
