.. _Muon_Analysis_ARGUS-ref:

Muon Unscripted Testing: ARGUS
==============================

.. contents:: Table of Contents
   :local:

Introduction
------------

These tests will look at double pulse data.
This is when the same sample is exposed to two sequential pulses.
It is possible to fit to both pulses simultaneously.
The fitting function, :math:`f`, as a function of time, :math:`t`, obeys

.. math::

	f(t) = f(t+\Delta),

where :math:`\Delta` is the time between the first and second pulse.

------------------------------------

.. _single_pulse_test:

Single Pulse Test
-----------------

**Time required 5 minutes**

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **ARGUS**, found in the *Home* tab
- Load run ``71799``
- Go to the **Grouping** tab
     - click **Guess Alpha**, should get ``0.95``
- Load the next run
- Go to the **Fitting** tab
     - Add a **StaticKuboToyabeTimeExpDecay** and **FlatBackground**
     - Click the **Fit** button
- Expected Values are (similar):
	- **A:** ``0.13``
	- **Delta:** ``0.52``
	- **Lambda:** ``0.11``
	- **A0:** ``0.07``

------------------------------------

.. _double_pulse_test:

Double Pulse Test
-----------------

**Time required 5 minutes**
This users the same sample as the single pulse test

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **ARGUS**, found in the *Home* tab
- Load run ``71796``
- Set to **Double Pulse**
- Go to the **Grouping** tab
     - click **Guess Alpha**, should get ``0.93``
- Load the next run
- Go to the **Fitting** tab
     - Add a **StaticKuboToyabeTimeExpDecay** and **FlatBackground**
     - click the **Fit** button
- Expected Values are (similar):
	- **A:** ``0.12``
	- **Delta:** ``0.55``
	- **Lambda:** ``0.13``
	- **A0:** ``0.09``

- These values are slightly different from the single pulse results
- Load run ``71796``
- Go to the **Home** tab
	- Change back to **Single Pulse**
	- Untick the **Time zero** and set the value to **0.493**
- Go to the **Grouping** tab
	- Click **Guess Alpha**, should get ``0.91``
- Load the next run
- Go to the **Fitting** tab
	- Add a **StaticKuboToyabeTimeExpDecay** and **FlatBackground**
	- Click the **Fit** button
- Expected Values are (similar):
	- **A:** ``0.12``
	- **Delta:** ``0.59``
	- **Lambda:** ``0.14``
	- **A0:** ``0.1``
