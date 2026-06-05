.. _Muon_Analysis_ARGUS-ref:

Muon Unscripted Testing: ARGUS
==============================


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

Single Pulse Test
-----------------

**Time required 5 minutes**

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **ARGUS**, found in the *Home* tab
- Load run ``71799``
- Go to the **Grouping** tab
     - click **Guess Alpha**, should get ``0.95``
- Load the next run by clicking the ``>`` button (the run number should now be ``71800``)
- Go to the **Fitting** tab
     - Add a **StaticKuboToyabeTimesExpDecay** and **FlatBackground**
     - Click the **Fit** button
	 - The fit should look reasonable

------------------------------------

Double Pulse Test
-----------------

**Time required 5 minutes**
This users the same sample as the single pulse test

- Open **Muon Analysis** (*Interfaces* > *Muon* > *Muon Analysis*)
- Change *Instrument* to **ARGUS**, found in the *Home* tab
- Load run ``71796``
- Set to **Double Pulse**
- Go to the **Grouping** tab
     - Click **Guess Alpha**, should get ``0.93``
- Load the next run by clicking the ``>`` button (the run number should now be ``71797``)
- Go to the **Fitting** tab
     - Add a **StaticKuboToyabeTimesExpDecay** and **FlatBackground**
     - Click the **Fit** button
	 - The fit should look reasonable
