.. _muon_grouping_tab-ref:

Grouping Tab
------------

.. image::  ../../images/muon_interface_tab_grouping.png
   :align: right
   :height: 400px

**Load** Allows a grouping and pairing table xml file to be selected to load.

**Save** Saves the current grouping and pairing tables to an xml file.

**Clear** Clears all groups and pairs from the current tables.

**Default** Loads in the default groups and pairs for the instrument.

Grouping Table
^^^^^^^^^^^^^^

**Analyse (plot/fit)** Whether the selected group is included in the plot and fitting.

**Plus and Minus** Add or remove groups.

**Context Menu Pair Selection** Right clicking over the grouping table, when two groups are selected, allows a pair to be created with a specified name.

**Group Asymmetry Range** Controls the range to use when estimating the group asymmetry. The values are used as ``StartX`` and ``EndX`` in :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>`. By default this range is the first good data and end time value from the file
but may be overridden here if required.


Pairing Table
^^^^^^^^^^^^^

**Analyse (plot/fit)** Whether the selected pair is included in the plot and fitting.

**Plus and Minus** Add or remove pairs.

**Guess Alpha** Estimates the alpha for the current pair and recalculates.

Multi-Period Data
^^^^^^^^^^^^^^^^^^^

For multi period data, the summed and subratcted periods are specfied at the bottom of the tab. These periods will be used for all 
the groups and pairs.

Used By
^^^^^^^

:ref:`Muon Analysis <MuonAnalysis_2-ref>`,
:ref:`Frequency Domain Analysis <Frequency_Domain_Analysis_2-ref>`