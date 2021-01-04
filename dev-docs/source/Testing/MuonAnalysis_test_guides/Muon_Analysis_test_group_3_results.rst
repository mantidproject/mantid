.. _Muon_Analysis_TestGuide_3_Results-ref:

==========================================================
Muon Analysis Unscripted Testing: Group 3 (Results Tables)
==========================================================

.. contents:: Table of Contents
    :local:
    
Introduction
------------

These are unscripted tests for the :program:`Muon Analysis` interface.
The tests here in group 3 are concerned with generating results tables from 
fits that have previously been run. In this case, the fits previously run are 
those in :ref:`Muon_Analysis_TestGuide_2_Fitting-ref`, so those tests need to 
be run before these ones. Once you have comepleted group 2 tests, go to the 
**Reults** tab.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

-----------

.. _test_1:

Test 1: Individual And Co-Added Fits
------------------------------------

**Time required 5-10 minutes**

- In the **Results** tab, in the top log values table, check **run_number** 
  and **sample_temp**
- Change **Function Name** to be **FlatBackground,Abragam** or equivalent
- The bottom table should have all fit workspaces which used both 
  **Flat Background** and the **Abragam** functions
- Click **Unselect All**
- Change **Table Name** to ``IndividualFitsTable``
- Check all boxes next to workspaces which are individual fits, including the 
  co-added workspace
- This should look a little something like this:

.. image:: /images/MuonAnalysisTests/results_test1_table.png
	:align: center
	:alt: results_test1_table.png
	:width: 600

- Click **Output Results**
- In the main workbench window, in the workspace toolbox, open the results 
  table
- View the table
- The table should hae four rows
- **sample_temp** should be correctly expresses as a range ``180-200`` for the 
  co-added runs
- There should be several pairs of columns, one for each parameter and one for 
  the error of the parameter, plus a final column **Cost function value**
- The table should look similar to this:

.. image:: /images/MuonAnalysisTests/results_test1.png
  :align: center

Test 3: Sequential fit of simultaneous fits
-------------------------------------------

- For this test we will use the same **Fucntion Name** as in :ref:`test_1` 
  (*FlatBackground,Abragam*)
- Change the **Table Name** to ``SeqFitOfSimFits``
- Click **Unselect All**
- Check the three workspaces that were simultaneous fits over groups and 
  sequentially fit for all runs
- At this point the table should look something like this:

.. image:: /images/MuonAnalysisTests/results_test2_table.png
	:align: center
	:alt: results_test2_table.png
	:width: 600
	
- Click **Output Results**
- In the main workbench window, in the workspace toolbox, open the results 
  table

- The top box should have extra log values for ``group`` and ``period`` - select ``group`` (as well as ``run_number``)
- Keep all runs selected and create table, this time with the name ``ResultsTable2``.
- There should be six rows in all, two per run (one for fwd, one for bwd).

Like this:

.. image:: /images/MuonAnalysisTests/results_tests2and3.png
  :align: center

Test 4: Simultaneous fits
-------------------------
- Select the "Simultaneous fits" radio button above the centre group box.
- Keep the first label, "20918", selected in the drop-down list. (This was the fit across groups in the same run)
- Create the table with the name ``Results20918``
- There should be one row per run
- For the global parameters (``f1.A``, ``f1.Omega``, ``f1.Phi``, ``f1.Sigma``), note the error is non-zero for the first run and zero for the others as they were all fitted together.

Test 5: Simultaneous fits (2)
-----------------------------
- Now in the drop-down list next to the "Simultaneous fits" radio button, select the label "20918-20", which was the fit across runs.
- Create the table with the name ``Results20918-20``
- In the drop-down list, select the label "MUSRlabel", the fit across periods for MUSR data. Have the ``period`` log value selected in the top box for this one, and create the table ``ResultsMUSR``.

Should look like this:

.. image:: /images/MuonAnalysisTests/results_tests4and5.png
  :align: center

Test 6: Multiple simultaneous fits
----------------------------------
- Select the last radio button, "Multiple", above the centre group box.
- Three simultaneous fit labels are listed: 20918, 20918#2, 20918-20 and MUSRlabel.
- Note that they have different colours, because the number of datasets and fit models differ. You should not be able to create a table with all of these selected.
- Select just the first two, i.e. "20918" and "20918#2", log values ``run_number`` and ``sample_temp``, and create the table.
- There should be columns for label, run number/sample temp (with correct ranges) and fit parameters.
- Non-global parameters should have a column for each dataset - so ``f0.f0.A0``, ``f1.f0.A0``, ``f2.f0.A0``
- Global parameters should share a column - so just ``f1.A``, ``f1.Omega`` etc.

Should look like this:

.. image:: /images/MuonAnalysisTests/results_test6.png
  :align: center

