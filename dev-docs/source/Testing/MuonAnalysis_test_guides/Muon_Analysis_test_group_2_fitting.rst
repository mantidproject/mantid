.. _Muon_Analysis_TestGuide_2_Fitting-ref:

===================================================
Muon Analysis Unscripted Testing: Group 2 (Fitting)
===================================================

.. contents:: Table of Contents
    :local:
    
Introduction
------------

These are unscripted tests for the :program:`Muon Analysis` interface.
The tests here in group 2 are concerned with the different types of fits 
possible through the **Fitting** and **Sequential Fitting** tabs. In each test, 
the fit should succeed without a crash - the results will be tested later, in 
:ref:`Muon_Analysis_TestGuide_3_Results-ref`.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

-----------

.. _test_1:

Test 1: Individual Fit
----------------------

**Time required 5-10 minutes**

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Go to the **Home** tab, select instrument **EMU** and load ``20918``
- Go to the **Fitting** tab
	- Right click the empty table area; Select **Add Function**
	- Add a **Flat Background** (*Background* > *Flat Background*)
	- Similarly, add **Abragam** (*Muon* > *MuonSpecific* > *Abragam*)
	- Set ``Omega = 8.5`` and ``Tau = 0.4``
	- Click **Fit**
- The fit should look something like this:

.. image:: /images/MuonAnalysisTests/fitting_test1.png
	:align: center
	:alt: fitting_test1.png
	
- In the bottom table, set ``Time Start = 1`` and ``Time End = 5``
- Click **Fit** again
- The fit curve should be updated with the new time constraints and look like 
  this:	
  
.. image:: /images/MuonAnalysisTests/fitting_test1_start_end_time.png
  :align: center
  :alt: fitting_test1_start_end_time.png

-----------
  
.. _test_2:
  
Test 2: Sequential Fit
----------------------

**Time required 5 minutes**

- Following the set up from :ref:`test_1`, load runs ``20918-20`` and reset 
  ``Time Start = 0.113`` and ``Time End = 15``
- Go to the **Sequential Fitting** tab
	- Click **Sequentially fit all**

-----------
	
.. _test_3:
	
Test 3: Co-added Fit
--------------------

- Following :ref:`test_2`, in the *Loading* section, check **Co-Add** to load 
  ``20918-20`` as one run
- Go to **Fitting** tab
	- In the **Select Workspace** drop-down, there should only be one workspace
	  ; ``EMU00020918-20; Pair Asym; long; MA``
	- Click **Fit** as before. The plot should be updated
	
.. image:: /images/MuonAnalysisTests/fitting_test3.png
	:align: center
	:alt: fitting_test3.png

-----------

.. _test_4:
	
Test 4: Simultaneous Fit Across Runs
------------------------------------

- Following :ref:`test_3`, uncheck **Co-Add**
- Go to the **Fitting** tab
	- Check the **Simultaneous fit over** checkbox, and change from **Run** 
	  to **Group/Pair**
	- The **Display parameters for** drop down should contain three workspaces
	- Using the same fit function as before, check the **Global** checkbox for 
	  the parameters **A**, **Omega**,**Phi** and **Sigma**
	- Click **Fit**
	- Use the **<<** and **>>** buttons, or drop-down list, to see the fitted 
	  parameters for each run in the function browser.
	- The fit should look something like this:
	
.. image:: /images/MuonAnalysisTests/fitting_test4.png
	:align: center
	:alt: fitting_test4.png

-----------
	
Test 5: Simultaneous Fit Across Groups
--------------------------------------

- Load run ``20918``, keeping the same set up as before in :ref:`test_4`
- Go to the **Grouping** tab
	- Uncheck **Analyse (plot/fit)** for the pair **long**, and check 
	  **Analyse (plot/fit)** for both groups; **fwd** and **bwd**
	- Keep fit function and global parameters as before
	- Change from **Group/Pair** to **Run**
	- The **Display parameters for** drop down should contain two workspaces
	- Click **Fit**
	- The fit won't be very good but it shouldn't crash

Test 6: sequential fit of simultaneous fits
-------------------------------------------
- Keep the same setup as Test 5, i.e. Runs="20918" and "All Groups" selected.
- Click *Fit/Sequential fit* to launch the dialog.
- If offered the choice, choose not to overwrite the label.
- Dialog should appear. In this new dialog (not the interface underneath):

  - Runs = "20918-20"
  - Label = "LabelSeq"
  - Hit "Start"

- This should fit the ``fwd`` and ``bwd`` groups simultaneously for each run 20918, 20919, 20920 in sequence.


Test 7: simultaneous fit across periods
---------------------------------------
The data used so far is single period, so here we will use MUSR data from the unit test data.

- Go back to the *Home* tab, set instrument to MUSR.
- Load run 15189 and switch to *Data Analysis* tab.
- (If any fit curves are still displayed, clear them with :menuselection:`Display --> Clear fit curves`).
- Two extra rows (``Selected Periods`` and ``Periods to fit`` ) should have appeared in the data table.  multi-period data.
- Note two points:

  - "All Pairs" should be selected - because "long" was loaded on the *Home* tab.
  - In the *label* box, the previous label "20918#2" has **not** been updated. This is because it contains a non-numeric character, so is assumed to be a user-set label (this is the intended behaviour).

- Set label to "MUSRlabel"

- Set fit function to ``LinearBackground`` (clear any existing function).
- Fit - periods will be fitted simultaneously.

Test 8: TF Asymmetry fit
-------------------------
- Go back to the *Home* tab and load run 62260.
- In the *Data Analysis* tab, set the "Groups/Pairs to fit" to "Custom".
- A pop-up should appear and make sure that only "fwd" is ticked.
- Close the pop-up.
- Clear the fitting functions.
- Add a "GausOsc" function.
- Set "Frequency" to 1.3.
- Enable "TF Asymmetry Mode".
- Run a fit.
- Look at the fitting parameters and notice that the "Flat Background" is non-zero (larger than the error).
- Disable "TF Asymmetry Mode".
- Remove the fitting functions.
- Add a "GausOsc" function and set "Frequency" to 1.3.
- Then add a "Flat Background" to the fitting functions.
- Run a normal Fit.
- The "Flat Background" should now have a value less than one.

Test 9: Multiple TF Asymmetry fits
----------------------------------
- Go back to the *Home* tab and load run 62261.
- Go to the *Data Analysis* tab.
- Clear the fitting functions.
- In "Groups Pairs to fit" select "All Groups".
- Add a "GausOsc" function with "Frequency" set to 1.3.
- Enable TF Asymmetry mode.
- Tick the "Global" box for "Frequency" and "Sigma".
- Fit.
- Check that all values for the flat background are different to each other.
