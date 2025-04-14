.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to combine a pair of :ref:`Table Workspaces <Table Workspaces>`
into a single table. The current algorithm is very lightweight and is intended for situations
where multiple tables are produced by the same processing pipeline, but ultimately the output
of all the generated data in a single table would suffice. As such, the algorithm requires that
the Column names match exactly, are in the exact same order and that corresponding columns have
identical data types.

The currently supported data types are ``double``, ``int``, ``bool``, ``float``, ``string``,
``size_t``, and ``V3D``.

.. categories::

.. sourcelink::
