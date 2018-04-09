.. _BatchWidget:

=============================
Batch Widget Developer Manual
=============================

.. contents:: Contents
   :local:

The Batch Widget is a hierarchical grid-based widget designed for interfaces which need a
spreadsheet-like interface for configuring and indicating the status of multiple (batch) reduction
jobs.


========
Concepts
========

Row Location
^^^^^^^^^^^^

A row location is the notion of an individual table row's position within the table. Since the
table is unlike a traditional spreadsheet in that it is hierarchical the table is more like a tree
of rows where all non-leaf nodes can have any number of children.

In practice some interfaces, such as Reflectometry are likely to want to constrain the
number of children or depth of the tree, and the batch widget has mechanisms for performing this.

Currently a row location is represented as a path from the root node to the row node in question,
this is actualised in the code as an ordered list of integers where each element represents the index
of the next node in the path relative to it's predecessor in the path. Some example nodes, their
corresponding paths, and their representations are illustrated in the diagram below.
