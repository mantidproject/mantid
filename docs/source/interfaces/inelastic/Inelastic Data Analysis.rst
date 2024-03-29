﻿.. _interface-inelastic-data-analysis:

Inelastic Data Analysis
=======================

Overview
--------

The Inelastic Data Analysis interface is a collection of tools within Mantid
for analysing reduced data from indirect geometry spectrometers, such as IRIS and
OSIRIS and data from direct geometry spectrometer LET.

The majority of the functions used within this interface can be used with both
reduced files (*_red.nxs*) and workspaces (*_red*) created using the Indirect Data
Reduction interface or using :math:`S(Q, \omega)` files (*_sqw.nxs*) and
workspaces (*_sqw*) created using either the Indirect Data Reduction interface or
taken from a bespoke algorithm or auto reduction.

Four of the available tabs are QENS fitting interfaces and share common features and
layout. These tabs are documented in :ref:`QENS Fitting <QENS-fitting-ref>`.

These interfaces do not support GroupWorkspaces as input.

.. interface:: Data Analysis
  :width: 750
  :align: center

Action Buttons
~~~~~~~~~~~~~~

There are several buttons on the bottom left of every tab in the interface. These are:

Settings
  Opens the :ref:`Settings <inelastic-interface-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Py
  Exports a Python script which will replicate the processing done by the current tab.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

.. categories:: Interfaces Inelastic
