.. _Muon_Analysis_FDA-ref:

Muon Unscripted Testing: FDA (Frequency Domain Analysis)
========================================================

.. contents:: Table of Contents
    :local:

Introduction
------------

These are unscripted tests for the :program:`Frequency Domain Analysis` interface.
The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

------------------------------------

.. _FFT_test:

FFT Test
--------

- Open **Frequency Domain Analysis** (*Interfaces* > *Muon* > *Frequency Domain Analysis*)
- Change *Instrument* to **MUSR**, found in the *Home* tab
- In the loading bar enter ``62260``
- Go to the **Transform** tab
    - Set the workspace to "MUSR00062260; Group; bkwd; Asymmetry; FD"
    - Click the calculate FFT button and a plot will appear
    - The plot window will show a broad peak
- In the **Fitting** tab it will contain 3 workspace ending in `Re_unit_MHz` (real), `Im_unit_MHz` (imaginary) and `mod_unit_MHz` (modulus)
- Go to the **Transform** tab
    - The "Apodization Function" determines the amount of smoothing of the data
    - Set the "Apodization Function" to ``None`` and press calculate
    - The plot will show a large peak at 0 and then lots of noise
    - Set the "Apodization Function" to ``Gaussian`` and press calculate
    - There will be a clear peak
    - The "padding" adds zeros to the end of the time domain data set, to improve the sampling of the FFT
    - Set the xrange for the plot to be from ``0`` to ``2`` by changing the x min and x max values below the plot
    - Set the "padding" to zero and press calculate
    - The plot should still have a peak, but it will be less smooth
    - Set the "padding" to ``50`` and press calculate
    - The plot will now be nice and smooth
- At the top of the plotting window change the unit from "Frequency" to "Field", the data will have different x axis

------------------------------------

.. _phase_test:

Phasequad Test
--------------

Muon data from all of the detectors can be combined into a pair of lines if there is a strong frequency peak.
This is done by applying a phase shift to each of the detectors, such that they all have a phase of zero.
The data can then be summed.

- Open Frequency Domain Analysis (Interfaces > Muon > Frequency Domain Analysis)
- Change Instrument to MUSR, found in the Home tab
- In the loading bar enter 62260
- Go to the **Phase** tab
- Click "calculate phase table"
- Enter ``ptable`` as the name of the table
- Select the Phase Table from the menu below, the name of the table will contain ``ptable``
- Click the plus button, located in the left down of the tab, to enter a new Phasequad
- Enter ``pq`` as the name of the Phasequad
- Go to the transform tab
- Tick the **imaginary Workspace** option
- Select the real and imaginary parts of ``pq`` to be the **Workspace** and **Imaginary Workspace** respectively
- Click calculate

------------------------------------

.. _maxent_test:

Maxent Test
------------

Maxent calculates the frequency spectra and then converts using an FFT to compare with the time domain data.

- Open Frequency Domain Analysis (Interfaces > Muon > Frequency Domain Analysis)
- Change Instrument to MUSR, found in the Home tab
- In the loading bar enter 62260
- Go to the **Transform** tab
- Change the drop-down menu at the top of the interface to "MaxEnt"
- The interface should look different
- Click the Calculate MaxEnt button
- The calculate button will be disabled and cancel enabled
- Click the cancel button
- Click Calculate MaxEnt
- The plot should update to a mostly flat line with a peak
- Make sure everything in the top table is ticked
- Click calculate MaxEnt
- In the plotting window change the plot to ``Maxent Dual Plot``
- You will now see 5 plots (1 frequency and 4 time domain)
- In the ADS expand the ``MUSR62260`` group
- It will contain several workspaces
- The workspace that ends with ``phase_convergence`` will show a plot that tends to a single y value as x gets larger (just check a spectrum or two)
- The table that ends with ``dead_times`` will have two columns: spectrum number and dead time
- The table that ends with ``phase_table`` will have three columns: spectrum number, asymmetry, and phase
- The workspace that ends with ``reconstructed_spectra`` will look like the original data
