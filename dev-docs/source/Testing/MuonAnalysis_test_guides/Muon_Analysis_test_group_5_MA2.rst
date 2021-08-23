.. _Muon_Analysis_TestGuide_4_MA2-ref:

Muon Unscripted Testing: Group 5 PSI
==================================================

.. contents:: Table of Contents
    :local:

Introduction
^^^^^^^^^^^^

These are unscripted tests for PSI data.

The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

Tests
^^^^^

Setup:
-------

- Open Muon Analysis
- On the **Home** tab set the instrument to **PSI**
- The load current run button should be greyed out
- Load **dolly 1529** using the **Browse** button
- Set `Rebin` to `Fixed` and enter a value of `5`
- You will get 4 sets of data that curve upwards
- In the **Grouping** tab there will be 4 groups defined


Auto Background Corrections:
----------------------------

- Go to the **Corrections** tab
- For the ``Background`` select ``Auto``
- The plots will now average around 0
- The values should be (approximatley):
	- Forw: ``19.2``
	- Back: ``93.6``
	- Left: ``104.4``
	- Rite: ``77.2``
- Tick the ``use Raw`` box and the values will change
- The new values should be similar (within 1) to before



Flat Background Corrections:
----------------------------

- Change the ``Select Function`` to ``Flat Background``
- The curves will be curved upwards again and the table will report an error
- The **Corrections** tab will show a red asterisk
- Go to the **Home** tab
- Untick ``Time Zero``
- Switch the plot from ``Asymmetry`` to ``Counts``
- Set the ``Time Zero`` to be ``0``
- The plots will now show a large peak
- Using the options at the bottom of the plot, set the x range from ``0.0`` to ``1.0``
- Go to the **Corrections** tab
- Change the ``Start X`` to ``0.0``, it will automatically change to ``0.007``
- Change the ``End X`` to just before the peak in the plot
- This will fit a flat background to the data before the peak
- The results should be similar to before


Manual Background Corrections:
------------------------------

- Change the plot back to ``Asymmetry``
- Change the ``Background`` to ``Manual``
- Untick the ``Apply parameter change to all domains``
- Change some of the background values
- Go to the **Home** tab
- Tick the ``Time Zero`` box
- The plots that you changed will drift away from zero at large times
- The plots that you did not change will average around zero
