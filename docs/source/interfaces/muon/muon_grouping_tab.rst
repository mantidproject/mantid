.. _muon_grouping_tab-ref:

Grouping Tab
------------

.. image::  ../../images/muon_interface_tab_grouping.png
   :align: right
   :height: 400px

The grouping tab is used for selecting the data that will be analysed.
This information is presented in a series of tables.
If a row in a table is highlighted in yellow it means that for some of the runs the data (periods) does not exist.
If a row in a table is highlighted in red it means that the data (periods) does not exist for the loaded runs.

**Load** Allows a grouping and pairing table xml file to be selected to load.

**Save** Saves the current grouping and pairing tables to an xml file.

**Clear** Clears all groups and pairs from the current tables.

**Default** Loads in the default groups and pairs for the instrument.

Grouping Table
^^^^^^^^^^^^^^

The table contains the definitions of the groups.
The headers are:

**Group Name** The name of the group (this cannot be edited).

**Periods** A list of the periods to add together. Can be seperated by commas or a dash for an inclusive range.

**Analyse (plot/fit)** Whether the selected group is included in the transform (if available) plot and fitting.

**Detector ID's** The detectors contained within the group. Can be seperated by commas or a dash for an inclusive range.

**N Detectors** The total number of detectors in the group (this cannot be edited directly).

**Context Menu Pair Selection** Right clicking over the grouping table, when two groups are selected, allows a pair to be created with a specified name.

Underneath the table are a pair of buttons:

**Plus and Minus** Add or remove groups.

Then there are:

**Group Asymmetry Range** Controls the range to use when estimating the group asymmetry. The values are used as ``StartX`` and ``EndX`` in :ref:`EstimateMuonAsymmetryFromCounts <algm-EstimateMuonAsymmetryFromCounts>`. By default this range is the first good data and end time value from the file
but may be overridden here if required.


Pairing Table
^^^^^^^^^^^^^

The pairing table contains the muon pairs, see :ref:`MuonPairingAsymmetry <algm-MuonPairingAsymmetry>`.
The headers are:

**Pair Name** The name of the pair (This cannot be edited).

**Analyse (plot/fit)** Whether the selected pair is included in the transform (when avialable), plot and fitting.

**Group 1** The group to be used as forward in :ref:`MuonPairingAsymmetry <algm-MuonPairingAsymmetry>`.

**Group 2** The group to be used as backward in :ref:`MuonPairingAsymmetry <algm-MuonPairingAsymmetry>`.

**Alpha** The :math:`alpha` value using in :ref:`MuonPairingAsymmetry <algm-MuonPairingAsymmetry>`.

**Guess Alpha** Estimates the alpha for the current pair and recalculates using :ref:`AlphaCalc  <algm-AlphaCalc>`.

Underneath the table are two buttons:

**Plus and Minus** Add or remove pairs.


Difference Table
^^^^^^^^^^^^^^^^

The difference tables allow for the subtraction of the asymmetries of the groups or pairs.
The two radio buttons are used to select if to show the difference table for the pairs or the groups.
The headers are:

**Diff Name** The name of the difference (This cannot be edited).

**Analyse (plot/fit)** Whether the selected difference is included in the transform (when avialable), plot and fitting.

**Group 1** The group/pair to be used as the left hand side of the difference.

**Group 2** The group/pair to be used as bright hand side of the difference.

**Plus and Minus** Add or remove differences.


Used By
^^^^^^^

:ref:`Muon Analysis <Muon_Analysis-ref>`,
:ref:`Frequency Domain Analysis <Frequency_Domain_Analysis-ref>`
