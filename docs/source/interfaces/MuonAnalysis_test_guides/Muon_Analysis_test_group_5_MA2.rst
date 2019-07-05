.. _Muon_Analysis_TestGuide_4_MA2-ref:

Muon Unscripted Testing: Group 5 (Muon Analysis 2)
==================================================

.. contents:: Table of Contents
    :local:

Introduction
^^^^^^^^^^^^

These are unscripted tests for the :program:`Muon Analysis 2` interface in Workbench.
Several tests in this group involve repeating tests from the optional :program:`Muon Analysis` and will be called to by
reference to the documentation for those tests.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

Tests
^^^^^

Test 1:
-------

- Repeat test 1 from :ref:`Muon_Analysis_TestGuide_1_General-ref`.
- Instead of MUT, you should have the option to select PSI.
- When PSI is selected load current run should be greyed out.
- Repeat test 2 from :ref:`Muon_Analysis_TestGuide_1_General-ref`. the Rebin options are located on the home tab
- Using "Groups and Pairs" lelect each of the detector groups, the plot window should change to show each group.

Test 2:
-------

- Go to the grouping tab.
- There should be four groups: top, bkwd, bottom and fwd.
- Create a new group called all using the + at the bottom of the group list.
- Under Detector IDs for all set it to 1-64.
- go back to the home tab and change your group to all.
- The plot should be updates with the line `MUSR62260; Group; all; Asymmetry; Rebin; MA: spec`

Test 3
------

- Go back to the home tab.
- Change the instrument to EMU and load run 20918
- Go to the fitting tab
- using the function browser add a GausOsc function.
- Set the frequency of GausOsc to 1.4 and click fit.
- The plot should update with the fit function and the difference.
- Click Undo Latest Fit
- The plot should now only have the Pair Asym line and the values in the fit browser should should be returned to their values before the fit (frequency should be 1.4 again)

Test 4
------

- Change the function name to GausFit1 and refit EMU20918 again using the same function as before.
- Add a flat background to the fit function and change the name to GausFit2
- Click Fit.
- The fit line on the plot should have GausFit2 in its label.
- Click Undo Latest Fit.
- The fit line on the plot should have GausFit1 in its label.

Test 5
------

- Make sure that co-add in the loading box is not checked.
- Load runs EMU 20918-20
- EMU20918, EMU20919 and EMU20920 should all be plotted
- Switch from single fit to sequential fit.
- Fit the runs using the same flat background and GousOsc fitting function as before.
- The fit and difference for all three runs should be plotted
