.. _Muon_Analysis_TestGuide_4_MA2-ref:

Muon Unscripted Testing: Group 5 PSI
==================================================

.. contents:: Table of Contents
    :local:

Introduction
^^^^^^^^^^^^

These are unscripted tests for PSI data.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

Tests
^^^^^

Setup:
-------

- Open Muon Analysis
- On the **Home** tab set the instrument to **PSI**
- The load current run button should be greyed out
- Load **dolly 1529** using the **Browse** button
- You will get 4 sets of data that curve upwards
- In the **Grouping** tab there will be 4 groups defined


Test 1:
-------

- On the plotting window change the combobox from **Asymmetry** to **Counts**
- Select the **Autoscale y** option on the plotting window
- On the **Home** untick **Time zero** and change the value to `0.0`
- All of the data will now include a very large spike in the data
- Tick the **Time zero** checkbox and all of the data will go back to 4 decay curves
- Untick the **First Good data** and set the value to `1`
- All of the plots will now start at `1`