.. _InterfaceDocumentation:

=======================
Interface Documentation
=======================

.. contents::
  :local:

Summary
=======

This page deals with the specifics of how to document Custom Interfaces. For a more general guide to the Mantid documentation system see `Documentation Guide For Devs <DocumentationGuideForDevs.html>`__.

The ``interface`` Directive
===========================

This directive allows you to insert a screenshot of the interface into the documentation. It has one required argument: the name of the interface, as given by the interface's ``name()`` method. Several options, detailed below, may also be provided to further control the behaviour of this directive.

The inserted screenshots are generated automatically when the documentation is being compiled. This is preferable to screenshots taken manually as these will automatically stay up to date. 

Options
-------

widget
   You can give the name of a widget in the interface to only take a screenshot of that widget. If not given, a screenshot of the entire interface will be inserted.

align
   This specifies the alignment to use for the screenshot. Valid settings are *left*, *center*, and *right*. Defaults to *center*.

Examples
========

This inserts a screenshot of the Muon Analysis interface:

.. code-block:: rest

   .. interface:: Muon Analysis

This inserts a screenshot of the Muon Analysis interface's Grouping Options tab:

.. code-block:: rest

   .. interface:: Muon Analysis
      :widget: GroupingOptions

This inserts a screenshot of the main table in the ISIS Reflectometry custom interface, aligned to the right:

.. code-block:: rest

   .. interface:: ISIS Reflectometry
      :widget: viewTable
      :align: right
