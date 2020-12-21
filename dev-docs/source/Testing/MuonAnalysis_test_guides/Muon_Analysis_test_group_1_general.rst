.. _Muon_Analysis_TestGuide_1_General-ref:

===================================================
Muon Analysis Unscripted Testing: Group 1 (General)
===================================================

.. contents:: Table of Contents
    :local:

Introduction
------------

These are unscripted tests for the :program:`Muon Analysis` interface.
The tests here in group 1 are concerned with general data loading and 
processing, as well as with switching between the old and new fitting 
interfaces and fitting raw or binned data. The master testing guide is 
located at :ref:`Muon_Analysis_TestGuide-ref`.

------------------------

Test 1: Load Current Run
------------------------

**Time required 5 minutes**

.. note:: This test will only work if you are using Windows and are connected 
		to the ISIS network. You should also enable search of the archive in your 
		:ref:`manage user directories <ManageUserDirectories>`. If this is not the 
		case, go to :ref:`test_2`.

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- On the **Home** tab, in the *Instrument* section, select each ISIS muon 
  instrument in turn
- For each instrument, click the **Load Current Run** button in the *Loading* 
  section
- A datafile should be successfully loaded, and a plot produced, for every 
  instrument except **PSI**
- When the current run is loaded, check that the **<** and **>** buttons 
  on the interface cycle through recent runs. You shouldn't be able to 
  use the **>** button after loading the most recent run as there should be no 
  runs that exist after the current run

-----------
  
.. _test_2:
  
Test 2: Data Loading and Rebinning
----------------------------------

**Time required 5 minutes**

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- On the **Home** tab, in the *Instrument* section, set the instrument to 
  **EMU**
- Load run ``20918``
- The data should look like this:

.. image:: /images/MuonAnalysisTests/emu20918.png
  :align: center
  :alt: emu20918.png

- On the **Home** tab, under the *Rebin* section, change **Rebin** to 
  **Fixed** with steps of ``5``. The data should now look like this:

.. image:: /images/MuonAnalysisTests/emu20918_rebin.png
  :align: center
  :alt: emu20918_rebin.png

-----------------------------------
  
Test 3: Old style data analysis GUI
-----------------------------------

**Time required 5-10 minutes**

- Start with the same data loaded and rebin settings from :ref:`test_2`
- Go to the **Fitting** Tab
	- Right click the empty table area; Select **Add Function**
	- Add **Abragam** (*Muon* > *MuonSpecific* > *Abragam*)
	- Set ``Omega = 8.5`` and ``Tau = 0.5``
	- Click **Fit**
- The fit should be applied to the rebinned data

image apvwibeq

- Now tick the ``Fit to raw data`` box in the bottom section and fit again. This time it will fit to the raw data, but the rebinned data will still be shown in the plot.

imagdewfui

Test 5: New style data analysis GUI
-----------------------------------

- On the *Settings* tab, turn "Enable multiple fitting" **on**.
- Go back to *Data Analysis*. The new UI should be there, and the fit function should have been cleared (so the Fit option is disabled until a new function is set up).
- Set up the Abragam function as in test 3 and fit.
- Try this with and without "Fit to raw data" as in test 4.
