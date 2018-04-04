.. Documentation master file
   It contains a hidden root toctree directive so that Sphinx
   has an internal index of all of the pages and doesn't
   produce a plethora of warnings about most documents not being in
   a toctree.
   See http://sphinx-doc.org/tutorial.html#defining-document-structure

.. _contents:

=======================
Developer Documentation
=======================

These pages contain the developer documentation for mantid version |release|.

======
Guides
======

.. toctree::
   :hidden:

   GettingStarted

:doc:`GettingStarted`
   Describes the process of obtaining and building the mantid code base

===================
Component Overviews
===================

.. toctree::
   :maxdepth: 1

   AlgorithmMPISupport
   IndexProperty
   ISISSANSReductionBackend
