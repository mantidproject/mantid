.. _Muon_Analysis_TestGuide-ref:

Muon Analysis Unscripted Testing
=================================

.. contents:: Table of Contents
    :local:
    
Preamble
^^^^^^^^^
This document is intended for developers to use for unscripted testing of the :program:`Muon Analysis` GUI.
User documentation for this interface can be found at :ref:`Muon_Analysis-ref`.

Testing here mostly focuses on the changes for Mantid 3.8, but includes other functionality as well.
The tests follow real use cases provided by scientists and are intended to exercise all the interface's functionality.
As changes are made to the interface and features added, anything for which it is not possible to write an automated test should have a manual test added to this list.

.. note:: The tests here are grouped in three sections. The test groups should be run in order and, within each test group, the individual tests should also be run in order. This is because, to avoid duplication, some tests rely on previous tests having been run first.

Common setup
^^^^^^^^^^^^
- Set your facility to ISIS
- Ensure the files ``EMU00020918-20`` and ``MUSR00015189`` are in Mantid's path
- Open :menuselection:`Interfaces --> Muon --> Muon Analysis`
- At this point, before loading any data, the *Grouping Options* and *Data Analysis* tabs should be disabled.

Group 1: Data loading and old/new fitting UI
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Test instructions for group 1 can be found at :ref:`Muon_Analysis_TestGuide_1_General-ref`.
These test general data loading and processing, and also switching between the old and new fitting user interfaces.

Group 2: Fitting tests
^^^^^^^^^^^^^^^^^^^^^^

Test instructions for group 2 can be found at :ref:`Muon_Analysis_TestGuide_2_Fitting-ref`.
These test the different types of fits. The fit should succeed in each without a crash.
The *results* will be tested later, in the results table test section (group 3).

Group 3: Results table tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Test instructions for group 3 can be found at :ref:`Muon_Analysis_TestGuide_3_Results-ref`.
These test the generation of results tables from the fits performed in the tests in group 2.

.. categories:: Interfaces Muon
