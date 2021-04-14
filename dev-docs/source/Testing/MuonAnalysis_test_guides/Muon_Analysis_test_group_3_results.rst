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
- The bottom table should have all fit workspaces that used both
  **Flat Background** and the **Abragam** functions
- Click **Unselect All** for the workspaces table
- Change **Table Name** to ``IndividualFitsTable``
- Check all boxes next to workspaces that are individual fits, including the
  co-added workspace
- This should look a little something like this:

.. image:: /images/MuonAnalysisTests/results_test1_table.png
	:align: center
	:alt: results_test1_table.png
	:width: 600

- Click **Output Results**
- In the main workbench window, in the workspace toolbox, open the results
  table
- The table should have four rows, one for each individual fit
- **sample_temp** should be correctly expresses as an average for the co-added
  runs, in this case the average is ``190`` (``180+190+200/3``)
- There should be several pairs of columns, one for each parameter and one for
  the error of the parameter, plus a final column called
  **Cost function value**
- The table should look similar to this:

.. image:: /images/MuonAnalysisTests/results_test1.png
  :align: center

Test 2: Sequential Fits And Simultaneous Fits
---------------------------------------------

*Time required 5-10 minutes*

- For this test we will use the same **Function Name** as in :ref:`test_1`
  (*FlatBackground,Abragam*)
- Change the **Table Name** to ``SeqFitOfSimFits``
- Click **Unselect All** for the workspaces table
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
- There should be 3 rows
- In particular, there should be only one column for ``A``, ``Omega``, ``Phi``
  and ``Sigma`` (plus one for each paramaters error) as these paramaters are
  global
- There should be a column for ``f0.f0.A0`` and ``f1.f0.A0`` as this was not
  a global paramater, and similarly for the ``Tau`` Parameter. They should all
  have their own error associated with them
- The table should look similar to this:

.. image:: /images/MuonAnalysisTests/results_test2.png
  :align: center
	:alt: results_test2.png
	:width: 600

Test 3: TF Asymmetry Results
----------------------------

*Time required 5-10 minutes*

- Click **Unselect All** for the log values table
- Now Select **Field_Danfysik**
- Change **Function Name** to be **GausOsc,TFAsymmetry** or equivalent
- There is one workspace which was fit simultaneously so will not be included
  in this results table. Deselect that worksapce
- Change the table name to ``TFAsymmetryFits``
- The table should look like this:

.. image:: /images/MuonAnalysisTests/results_test3_table.png
	:align: center
	:alt: results_test3_table.png
	:width: 600

- Click **Output Results**
- There should be 12 rows and a column for **Field_Danfysik**, like this:

.. image:: /images/MuonAnalysisTests/results_test3.png
	:align: center
	:alt: results_test3.png
	:width: 600


