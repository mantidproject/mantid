.. _the_tabs_home:

===============
The Tabs - Home
===============

.. index:: The Tabs - Home

.. contents:: Table of Contents
  :local:

Home
====

When launched, the Muon Analysis GUI defaults to the Home tab. This tab allows:

* Data files to be loaded.
* Run information to be viewed.
* `Time zero`, `first good data` and `last good data` to be set.
* `Deadtime corrections` to be set
* Data binning to be modifed.

Loading a Workspace
-------------------

To load a file either: `Browse` or `Load Current Run` or simply type a run number (assuming you have defined the directory(ies) in which your files are stored)
When typing a run number, or using the 'Load Current Run' option, first select the desired instrument from the dropdown list.

To demonstrate:
1. Select 'MUSR' in the instrument drop-down menu
2. Type run number '24563' in the Loading section and press enter, note this can only be done if the correct reference material folder was selected in :ref:`getting_started`. This process shown in Figure 16.

*NB the plot's appearance will vary based on the Time axis and Rebin data values as described later in this section and* :ref:`other_mantid_functions`

.. figure:: /images/thetabshomefig16.gif
    :align: center

    Figure 16: How to load a workspace in the Muon Analysis GUI.

Regardless of the data input method used, the 'Time Zero' (:math:`{\mu s}`), 'First Good Data' (:math:`{\mu s}`) and  'Last Good Data' (:math:`{\mu s}`)
values are automatically updated. These values have been determined by the instrument scientist during instrument calibration periods, and are stored in the header
block of the raw .nxs data files, which are saved once a measurement is finished. Once a data file has been successfully read, a new plot window like the one shown in Fig. 2(b) will appear.

*NB: when browsing for files multiple files such as `15190,15193`  or a string like `15190-3` can be selected (the latter would load runs from `15190` to `15193`).
The selected files will each be loaded into a different workspace.*


Data Binning
------------

Data can be re-binned via the home tab by using the `Rebin` section. The options are `None` for no binning, `Fixed` to use a
given value (entered in the`Steps` box to the right) or `Variable`, for binning
with various steps. When entering values in the `Steps` box, do so as for parameters in the
:ref:`Rebin <algm-Rebin>` algorithm.

For example, to set the plot to a fixed bin-width of choice, follow the instructions below

1.  Load HIFI run number `00062798` (as described above).
2.  In the `Rebin` section of the Home tab, use the drop-down menu and change its value from `None` to `Fixed`.
3.  In the box adjacent to it, input a suitable value - `10` is suggested - and press enter. This will cause a new workspace, `HIFI62798; Pair Asym; long; Rebin; MA` to appear in `HIFI62798`.
4.  The effect of rebinning is best viewed on only a certain portion of the data, use the Figure options as described in the Overlaying and Styling Plots section of :ref:`other_mantid_functions`
5.  Go to the ADS and plot `HIFI62798; Pair Asym; long; MA`.
6.	Navigate to, `HIFI62798; Pair Asym; long; Rebin; MA`, then right click it and select `Plot` > `Overplot spectrum with errors`. The rebinned data should appear over the unbinned dataset. If this does not happen, check the Loading Data section of :ref:`other_mantid_functions` and ensure the plotting has been carried out correctly. An example of this process is shown in Figure 17 below.

.. figure:: /images/thetabshomefig17.gif
    :align: center

    Figure 17: How to re-bin data, in this example from a width of `10` to `20` on the `HIFI00062798`
    dataset.

A summary of each input field in the Home tab, and a description of its function(s) can be found in :ref:`Muon_Analysis-ref` under Home.
