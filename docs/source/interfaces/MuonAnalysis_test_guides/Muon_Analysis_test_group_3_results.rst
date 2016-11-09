.. _Muon_Analysis_TestGuide_3_Results-ref:

Muon Analysis Unscripted Testing: Group 3 (Results Tables)
==========================================================

.. contents:: Table of Contents
    :local:
    
After running all the fits above, go to the *Results Table* tab.

Test 1: individual fits
-----------------------
- Top group box: select ``run_number`` and ``sample_temp`` as logs
- Centre group box: keep the default, which should be the *Individual fits* radio button with both individual fits selected
- Create the table with the default name ``ResultsTable``.
- Table should have two rows, one for runs 20918-20 (co-added) and one for run 20918.
- Sample temp should be expressed correctly as a range "180 to 200" for co-added runs
- Should be one pair of columns for each parameter + error, plus a final column for "cost function value"

Test 2: Sequential fit (simple)
-------------------------------
- Top group box: keep same logs as before
- Select the *Sequential fits* radio button in the centre group box, and "Label" in the drop-down, *i.e.* the first sequential fit you did. Keep all three runs included.
- Create the table - check that, if you don't change the name, it warns you about overwriting.
- The table should have three rows, one for each run, with the correct sample log values (180, 190, 200).
- Again there should be one pair of columns for each parameter and error, and a cost function column at the end.

Test 3: Sequential fit of simultaneous fits
-------------------------------------------
- Now select "Label2", the sequential fit of simultaneous fits (from test 5 above), in the drop-down next to *Sequential fits* (see test 2).
- The top box should have extra log values for ``group`` and ``period`` - select ``group`` (as well as ``run_number``)
- Keep all runs selected and create table
- There should be six rows in all, two per run (one for fwd, one for bwd).

Test 4: Simultaneous fits
-------------------------
- Select the "Simultaneous fits" radio button above the centre group box.
- Keep the first "Label" selected in the drop-down list.
- Create the table
- There should be one row per run
- For the global parameters (``f1.A``, ``f1.Omega``, ``f1.Phi``, ``f1.Sigma``), note the error is non-zero for the first run and zero for the others as they were all fitted together.
- Try the second label ("Label#2"), which was the simultaneous fit across groups. Have the ``group`` log value selected (as well as ``run_number``). There will be one row for each group.
- Try the third label ("MUSRlabel"), the fit across periods for MUSR data. Have the ``period`` log value selected. 

Test 5: Multiple simultaneous fits
----------------------------------
- Select the last radio button, "Multiple", above the centre group box.
- Three simultaneous fit labels are listed: Label, Label#2, MUSRlabel.
- Note that they have different colours, because the number of datasets and fit models differ. You should not be able to create a table with all of these selected.
- Select just the first "Label", log values ``run_number`` and ``sample_temp``, and create the table.
- There should be columns for label, run number/sample temp (with correct ranges) and fit parameters.
- Non-global parameters should have a column for each dataset - so ``f0.f0.A0``, ``f1.f0.A0``, ``f2.f0.A0``
- Global parameters should share a column - so just ``f1.A``, ``f1.Omega`` etc.


