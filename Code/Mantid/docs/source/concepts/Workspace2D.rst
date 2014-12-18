.. _Workspace2D:

Workspace2D
===========

The Workspace2D is a Mantid data type for a
:ref:`MatrixWorkspace <MatrixWorkspace>`.

It consists of a workspace with 1 or more spectra. Typically, each
spectrum will be a histogram. For example, you might have 10 bins, and
so have 11 X-value, 10 Y-values and 10 E-values in a workspace.

In contrast to an :ref:`EventWorkspace <EventWorkspace>`, a Workspace2D
only contains bin information and does not contain the underlying event
data. The :ref:`EventWorkspace <EventWorkspace>` presents itself as a
histogram (with X,Y,E values) but preserves the underlying event data.



.. categories:: Concepts