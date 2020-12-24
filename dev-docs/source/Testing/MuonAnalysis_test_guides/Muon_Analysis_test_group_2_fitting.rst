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
:ref:`Muon_Analysis_TestGuide_3_Results-ref`. You will be using all the results 
from these tests as inputs for :ref:`Muon_Analysis_TestGuide_3_Results-ref` so 
**do not** clear or close the interface.

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

**Time required 5 minutes**

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

**Time required 5 minutes**

- Following :ref:`test_3`, uncheck **Co-Add**
- Go to the **Fitting** tab
	- Check the **Simultaneous fit over** checkbox, and change from **Run** 
	  to **Group/Pair**
	- The **Display parameters for** drop down should contain three workspaces
	- Using the same fit function as before, check the **Global** checkbox for 
	  the parameters **A**, **Omega**, **Phi** and **Sigma**
	- Click **Fit**
	- Use the **<<** and **>>** buttons, or drop-down list, to see the fitted 
	  parameters for each run in the function browser.
	- The fit should look something like this:
	
.. image:: /images/MuonAnalysisTests/fitting_test4.png
	:align: center
	:alt: fitting_test4.png

-----------

.. _test_5:
	
Test 5: Simultaneous Fit Across Groups
--------------------------------------

**Time required 5 minutes**

- Load run ``20918``, keeping the same set up as before in :ref:`test_4`
- Go to the **Grouping** tab
	- Uncheck **Analyse (plot/fit)** for the pair **long**, and check 
	  **Analyse (plot/fit)** for both groups; **fwd** and **bwd**
	- Keep fit function and global parameters as before
	- Change from **Group/Pair** to **Run**
	- The **Display parameters for** drop down should contain two workspaces
	- Click **Fit**
	- The fit won't be very good but it shouldn't crash
	
-------------------------------------------

Test 6: Sequential Fit of Simultaneous Fits
-------------------------------------------

**Time required 5 minutes**

- Load runs ``20918-20`` again
- Keep the same set up as :ref:`test_5` in the **Grouping** and **Fitting** 
  tab
- With **Simultaneous fit over** still checked, go to the 
  **Sequential Fitting** tab
	- Click **Sequentially fit all**
	- This should fit the **fwd** and **bwd** groups simultaneously for each 
	  run in sequence; ``20918``, ``20919``, ``20920``
- In the workspace toolbox there should be a group workspace for each run 
  which contains fitted data for both **fwd** and **bwd**

---------------------------------------
	  
Test 7: Simultaneous Fit Across Periods
---------------------------------------

**Time required 5 minutes**

The data used so far has been single period, so here we will use MUSR data 
which has multiple periods.

- Go back to the **Home** tab and set *Instrument* to **MUSR**
- Load run ``15189``
- Go to the **Grouping** tab
	- There should now be four groups, two **fwd** and two **bwd**, and two 
	  pairs
- Go to the **Fitting** tab	
	- Remove any existing functions (Right click *Composite function* > *Remove function*)
	- Make sure **Simultaneous fit over** is checked and is over **Run**
	- Add a **Linear Background** (*Background* > *Linear Background*)
	- Click **Fit**
- The fit should look something like this:

.. image:: /images/MuonAnalysisTests/fitting_test7.png
	:align: center
	:alt: fitting_test7.png
	
------------------------

Test 8: TF Asymmetry Fit
------------------------

**Time required 5-10 minutes**

- Now load run ``62260``
- There should be a warning to say **MainFieldDirection** has changed
- Go to the **Grouping** tab
	- Remove the rows from the table which are highlighted by right clicking 
	  the row then remove
	- Uncheck **Analyse (plot/fit)** for the pair, and check 
	  **Analyse (plot/fit)** for the **fwd** group
- Go to the **Fitting** tab
	- Clear the fitting function as before, and uncheck
	  **Simultaneous fit over**
	- Add **GausOsc** (*Muon* > *MuonGeneric* > *GausOsc*)
	- Set ``Frequency = 1.3``
	- In the bottom table, enable **TF Asymmetry Mode**
	- Click **Fit**
	- Look at the fitting parameters and see **Flat Background** is non-zero.
	- Instead disable **TF Asymmetry Mode**
	- Clear the remaining fitting functions
	- Add **GausOsc** (*Muon* > *MuonGeneric* > *GausOsc*) and set 
	  ``Frequency = 1.3`` as before
	- Now add **Flat Background** (*Background* > *Flat Background*)
	- Click **Fit**
	- Now check the parameters for flat background, they should be closer to 0
	
----------------------------------

Test 9: Multiple TF Asymmetry Fits
----------------------------------

**Time required 5 minutes**

- Load run ``62261`` (still using *Instrument* **MUSR**)
- Go to the **Grouping** tab
	- Check  **Analyse (plot/fit)** for both groups
- Go to the **Fitting** tab
	- Check **Simultaneous fit over** and make sure it is over **Run**
	- Clear all functions
	- Add **GausOsc** (*Muon* > *MuonGeneric* > *GausOsc*) and set 
	  ``Frequency = 1.3``
	- Tick the **Global** checkbox for **Frequency** and **Sigma**
	- Enable **TF Asymmetry Mode**
	- Click **Fit**
	- Check that all values for the flat background are different to each other
