.. _Muon_Analysis_MUSR-ref:

Muon Unscripted Testing: MUSR
=============================

.. contents:: Table of Contents
   :local:

Introduction
------------

Muon data is collected using 100's of detectors.
The detectors are positioned into a series of groups.
The data from all of the detectors in a group are combined into a data set.
These data sets are referred to as groups and are one of the primary ways to represent data.
The other way to represent data is called a pair.
A pair takes the ratio between the forward group, :math:`F`, and the backwards group, :math:`B`, and is given by

.. math::

    A = \frac{F-\alpha B}{F+\alpha B},

where :math:`\alpha` is the balance parameter and :math:`A` is the asymmetry.
The forwards and backwards groups are normally positioned opposite each other.
As the cone of muon decay emissions rotates the signal will move from one group to the other.
This setup is called a longitudinal field.

There is also a transverse field (TF) configuration.
This has four detector groups, arranged to form edges of a square.

-------------------------

.. _loading_test_MUSR:

Loading Data Test
-----------------

**Time required 15 - 20 minutes**

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **MUSR**, found in the *Home* tab
- If you are on Windows, click the ``Load Current Run`` button
- A plot should appear, the details are not important
- In the loading bar enter ``62250-1``
- Two lines will appear on the plot
- Tick the ``Co-Add`` option in the loading area
- There will now be one line on the plot
- Go to the **grouping tab**
	- You will see three tables, they are the groups, pairs and differences
	- Underneath the group table press the ``+`` button to create a group called "one"
	- A new group will appear in the table
	- Right click in the table and select ``Add Group``, call this one "two"
	- In the table, tick the ``Analyse`` option for groups "one" and "two", 2 lines will appear on the plot
	- In the pair table below, untick the ``Analyse``  option
	- The two remaining lines should be identical
	- In the ``Detector IDs`` column change the value for "two" to be ``2`` and the two groups will now look different
	- Select the groups "one" and "two", then right click and select ``Add Pair``
	- Call the pair "short", it will then appear in the pair table
	- Tick the ``Analyse`` option for the "short" pair
	- In the group table, untick the ``Analyse`` option for groups "one" and "two"
	- Change the value of ``Group 1`` to be "two", this will change the data and ``Group 2`` should update to be "one"
	- Click the ``+`` button on the difference table and create a pair called "pairDiff"
	- Tick the ``Analyse`` option for the "pairDiff" difference
	- Untick the ``Analyse`` option for the "short" pair
	- Changing the options for ``Pair 1`` and ``Pair 2`` will alter the data
	- Above the difference table, select the ``Groups`` radio button
	- Right click in the difference table and select ``Add diff``, call it "delta"
	- Tick the ``Analyse`` option for the "delta" difference (this is still technically a group)
	- Untick the ``Analyse`` option for the "pairDiff" difference
	- Press the ``Save`` button at the top of the tab and save the grouping as "MUSR_test"
	- Remove groups, pairs and differences by using either by selecting the row and pressing the ``-`` button or by right clicking the row and selecting ``Remove``
	- The GUI will not let you delete a group/pair that is in use elsewhere
	- Press the ``Load`` button and select "MUSR_test"
	- You will have 4 groups, 2 pairs, a difference of pairs and a difference of groups as before.
	- Press the ``Default`` button at the top of the tab
	- You will have two groups and one pair
- If you double click the tab heading, the tab will undock, pressing ``X`` will cause it to go back into the main GUI
- In the load bar enter ``62250``
- Press the ``>`` in the loading area and the next run will be loaded

----

Transverse Field Asymmetry Test
-------------------------------

**Time required 15 minutes**

This test will look at transverse field data.

- In the load bar enter ``62260``
- In the **grouping tab** you will see four groups and no pairs (you may need to press ``Default``)
- Go to the **fitting tab**
	- Right click the empty table area; Select ``Add Function``
	- Add ``GuasOsc``
	- Set ``Frequency = 1.3``
	- Add a ``FlatBackground``
	- Do a fit, notice that the value for the background is non-zero, when it should be
	- Remove the flat background
	- Change the ``Fitting type`` from "Normal" to "TF Asymmetry"
	- Press Fit
	- Change the ``Fitting type`` back to "Normal"
	- Add a ``FlatBackground``
	- Do a fit, notice that the value for the background is zero (the value is smaller than the error)

We can do a better fit by using simultaneous fitting

- Tick the ``Simultaneous over`` option
- In the fitting function, set ``Sigma`` and ``Frequency`` to global
- Press fit
- Inspect the flat background by using the arrows next to ``Display Parameters For``, all but one should be non-zero
- Remove the flat background
- Change the ``Fitting type`` from "Normal" to "TF Asymmetry"
- Press Fit
- Change the ``Fitting type`` back to "Normal"
- Add a ``FlatBackground``
- Do a fit, notice that the values for the background are zero (the value is smaller than the error)


Rebin and Fitting
-----------------
- In the load bar enter ``62260``
- On the **home tab** set ``Rebin`` to ``Fixed`` and enter a value of ``10``
- If you tick the ``Plot raw`` option the data will change
- Go to the **fitting tab**
	- Add a ``GausOsc``
	- Set ``Frequency = 1.3``
	- Make sure that ``Fit to Raw`` is ticked and the ``Plot raw`` option is unticked
	- Press fit, the fit will be much smoother than the data
	- Untick ``Fit to Raw`` and press fit
	- The data will now match the jagged data
