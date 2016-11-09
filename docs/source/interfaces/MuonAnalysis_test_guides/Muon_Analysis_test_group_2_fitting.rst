.. _Muon_Analysis_TestGuide_2_Fitting-ref:

Muon Analysis Unscripted Testing: Group 2 (Fitting)
===================================================

.. contents:: Table of Contents
    :local:
    
Test 1: individual fit
----------------------
- Go to *Data Analysis* tab. Expand the window if the function browser is too small (you can drag the bar underneath it to set relative sizes of the widgets).
- The loaded dataset should be pre-selected: "20918" in Runs and "long" ticked as the group.
- "Label" box and "Co-add/Simultaneous" radio buttons should be disabled
- Click and drag blue lines on graph, check start/end times are updated.
- Check the reverse - change start/end times and blue lines should be updated on graph.
- Set up the interface to look like this. Note the non-default values for ``f1.Omega`` and ``f1.Sigma``:

.. image:: ../../images/MuonAnalysisTests/test1.png
  :align: center

- Fit the data. Graph should be updated.

Test 2: sequential fit
----------------------
- With everything set up as in the picture above, click *Fit/Sequential fit*. (Runs should still be set to "20918", a single run).
- Dialog should appear. In the runs box **of the new dialog that appears**, type "20918-20" and hit Start
- Sequential fit of runs 20918, 20919, 20920 should happen one after the other

Test 3: co-added fit
--------------------
- Close the sequential dialog and return to the main interface.
- Now in the "Runs" box of MuonAnalysis's *Data Analysis* tab, type "20918-20" and hit return.
- Stale errors should be cleared from the function browser.
- "Co-add/Simultaneous" radio buttons should be enabled with "Co-add" selected
- "Label" box should still be disabled
- In the drop-down, there should only be one workspace (``EMU00020918-20; Pair; long; Asym; #1``)
- Fit as before. Graph should be updated.

Test 4: simultaneous fit across runs
------------------------------------
- With the same runs (20918-20) selected, select the "Simultaneous" radio button option.
- Drop-down list should have three workspaces in it now, for the three runs that will be fitted.
- Keep the same fit function, but use the "Global" checkboxes to mark ``A``, ``Omega``, ``Phi`` and ``Sigma`` as global.
- Fit the data. Note that plot will *not* be updated at present.
  (If you want to plot results, see the ``MuonSimulFit_Label`` workspace group)
- Use the ``<<`` and ``>>`` buttons, or drop-down list, to see the fitted parameters for each run in the function browser.

Test 5: simultaneous fit across groups
--------------------------------------
- Type "20918" only in the "Runs" box
- Select both "fwd" and "bwd" as groups
- Keep fit function and global parameters as before
- Fit data. It should warn you that "Label" has already been used - say no to overwriting and it should automatically increment the label.

Test 6: sequential fit of simultaneous fits
-------------------------------------------
- Keep the same setup as Test 5, i.e. Runs="20918", "fwd" and "bwd" selected as groups
- Click *Fit/Sequential fit* to launch the dialog
- If offered the choice, choose not to overwrite the label
- Dialog should appear. In this new dialog (not the interface underneath):

  - Runs = "20918-20"
  - Label = "Label2"
  - Hit "Start"

- This should fit the ``fwd`` and ``bwd`` groups simultaneously for each run 20918, 20919, 20920 in sequence.


Test of simultaneous fit across periods
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The data used so far is single period, so here we will use MUSR data from the unit test data.

- Close and open the Muon Analysis interface again. 
- This time, on the *Home* tab, set instrument to MUSR
- Load run 15189 and switch to *Data Analysis* tab
- An extra box should have appeared because this is multi-period data. Like this:

.. image:: ../../images/MuonAnalysisTests/multiperiod.png
  :align: center

- Check the boxes for periods 1 and 2.
- Set fit function to LinearBackground and the label to "MUSRlabel"
- Fit - periods will be fitted simultaneously

