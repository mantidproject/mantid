Inelastic Data Processor Testing
================================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_
- Make sure that the data set is on your list of search directories

Symmetrise tab
--------------

**Time required 5-10 minutes**

--------------


#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Symmetrise`` tab
#. On the File Input, click on ``Browse``, a dialog window should prompt.
#. Find the file ``irs26176_graphite002_red.nxs`` from the ISIS Sample Data and Load it.
#. The first spectrum should be rendered in the plot browser.
#. On the plot, drag and move the rightmost vertical green slider to be approximately at 0.4 on the x-axis, the value of ``Ehigh`` should change accordingly.
#. Move the other slider to be approximately at 0.2 on the x-axis, the value of ``Elow`` should change accordingly.
#. Click on the ``Preview`` button. The symmetrised plot should be rendered on the preview graph.
#. Click the ``Run`` button.
#. A Workspace named ``irs26176_graphite002_sym_pn_red`` should be created on the ADS.
#. On the Output widget, click on ``Plot Spectra`` button. A new plot window should appear with the same plot as the preview.
#. On the Symmetrise Property Browser: Change the value of ``Elow`` and ``Ehigh`` properties and see that the vertical green sliders.
   respond accordingly, but only when the input values are within range. It shouldn't be possible to set ``Elow`` to the right of ``Ehigh`` and viceversa.
   At the same time, it shouldn't be possible to enter negative numbers, or positive numbers larger than the maximum data range.
#. Change the value of ``Spectrum No`` property to 5 and see that the spectra on the plot is updated.
#. Change the ``ReflectType`` property from ``Positive to Negative`` to ``Negative to Positive``, the sliders should move to the negative range of the spectra.
#. Now ``Ehigh`` should be closer to 0 than ``Elow``. Repeat checks of step 12, noting that only negative inputs can be entered now.
#. Set ``Ehigh`` to -0.1 and ``Elow`` to -0.3 and click on the ``Preview`` button. The symmetrise plot should be rendered on the preview graph.
   The axis data range must go from -0.3 to 0.3.
#. Click on the ``Run`` button.
#. A Workspace named ``irs26176_graphite002_sym_np_red`` should be created on the ADS.
#. On the ADS, select ``irs26176_graphite002_sym_np_red`` and right-click to select ``Plot -> Spectrum...``. On ``Spectrum Numbers`` select 5.
   Check that the spectrum is the same as the one in the ``Preview`` plot of the symmetrise interface.

.. _symmetrise_inelastic_test:

:math:`S(Q, \omega)` tab
------------------------

**Time required 10-15 minutes**

--------------

1. ISIS data
############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``S(Q,w)`` tab
#. On the File Input, click on ``Browse``, a dialog window should prompt.
#. Find the file ``irs26176_graphite002_red.nxs`` from the ISIS Sample Data and Load it.
#. The data will be plot as a contour plot in the interface.
#. Set `Q Low`, `Q Width` and `Q High` to be 0.45, 0.05 and 1, respectively.
#. Click `Run` button.
#. There should be two new workspaces in the ADS with the prefixes ``_sqw`` and ``_rqw``.
#. On `Output` options of the interface. Select from the drop-down menu on the right of the ``Plot Spectra`` button the option to ``Open Slice Viewer`` and click.
#. A ``Slice Viewer`` window should prompt, check on the ``Slice Viewer`` that the spectra slices range in q-values from 0.45 to 1.
#. Back on the interface, check `Rebin in Energy` and change ``E Low`` and ``E High`` to be -0.2 and 0.2.
#. Click `Run`. A new rebinned workspace with suffix `_r` should appear on the ADS.
#. On `Output` options of the interface. Select from the drop-down menu on the right of the ``Plot Spectra`` button the option to ``Plot 3D Surface`` and click.
#. The resulting 3D plot should range in the Energy Transfer axis from -0.2 to 0.2 :math:`meV`.
#. Don't remove yet the workspace with suffix ``_sqw`` from the ADS as you will use it for the Moments interface test.
#. Repeat instructions 4 to 10, but loading instead the ``MAR27691_red.nxs`` file from the ISIS Sample Data set.

2. ILL data
###########

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``S(Q,w)`` tab
#. Open the Settings widget from the bottom left of the interface, and untick `Restrict allowed input files by name (recommended)`
#. On the File Input, click on ``Browse``, a dialog window should prompt.
#. Find the file ``243489-243506ElwinHeat_H2O-HSA-stw.nxs`` from the SystemTest data directory and Load it.
#. The data will be plot as a contour plot in the interface.
#. The `Q Low`, `Q Width` and `Q High` will be automatically set to 0.2, 0.05 and 2.25 respectively.
#. Click `Run` button.
#. There should be two new workspaces in the ADS with the prefixes ``_sqw`` and ``_rqw``.
#. Open the Settings widget from the bottom left of the interface, and tick `Restrict allowed input files by name (recommended)`

.. _sqw_inelastic_test:

Moments tab
-----------

**Time required 2-3 minutes**

--------------

1. Indirect data
################

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Moments`` tab
#. On the File Input, change the drop-down selector from ``File`` to ``Workspace``.
#. Select the previously generated workspace ``irs26176_graphite002_sqw`` from the selector.
#. First spectrum should be rendered on the plot widget, with two vertical sliders placed on the integration limits,
   the limits on the sliders and on the property browser should match.
#. Change the limits by moving the sliders to be at -0.2 and 0.2, respectively. On the property browser, ``EMin`` and ``EMax`` should be updated with the new limits.
#. Click the ``Run`` button.
#. The Preview plot should be updated with a representation of the first three moments with respect to Q. A workspace with the suffix `_Moments` should be generated on the ADS.
#. On the Output edit, the name of the output workspace should be greyed out. The spectra numbers box should also be disabled, and should read `0,2,4`. Click on the ``Plot Spectra`` button.
   A plot window should be generated with three of the calculated moments.

2. Invalid data
###############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Moments`` tab
#. On the File Input, click on ``Browse``, a dialog window should prompt.
#. Find the file ``MAR27698_red.nxs`` from the ISIS Sample Data and Load it.
#. Click the ``Run`` button.
#. There should be a red error message in the logger. No data should be plotted in the bottom embedded plot.


.. _moments_inelastic_test:

Elwin tab
---------

**Time required 3 - 5 minutes for each**

--------------

1. Direct data
##############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Elwin`` tab
#. Click on ``Add Workspaces``, a dialog window should prompt.
#. Enter ``MAR27691_red.nxs`` in ``Input file``. The table of the dialog should be populated with the ``MAR2791_red`` workspace.
#. Select the workspace from the table and click on ``Add Data``. Close the dialog.
#. Back on ``Elwin`` tab, click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Open the ``Add Workspaces`` dialog again, and in ``Input file`` choose ``browse``. Navigate to the ISIS-Sample data and select the two files, ``MAR27691_red.nxs`` and ``MAR27698_red.nxs`` using shift key.
#. Add the loaded workspaces
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name
#. In the main GUI right-click on ``MAR27691-27698_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`ln(Counts(microAmp.hour))^{-1}` vs :math:`Q2`

.. _elwin_inelastic_test:

2. Indirect data
################

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Click on ``Add Workspaces``, a dialog window should prompt
#. Enter ``irs26174_graphite002_red.nxs`` in ``Input file``. The table of the dialog should be populated with the ``irs26174_graphite002_red`` workspace.
#. Select the workspace from the table and click on ``Add Data``. Close the dialog.
#. Back on ``Elwin`` tab, click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Right-click on the ``irs26174_graphite002_red_elwin_eq`` workspace and ``Save Nexus``; save to a location of your choice. **NB** keep this workspace if you are doing the :ref:`QENS Fitting Manual Test <inelastic_qens_fitting_testing>`
#. Remove the Workspace from the interface by clicking on the ``Select All`` button and then the ``Remove Selected`` button.
#. Open the ``Add Workspaces`` dialog again, there should be a ``irs26174_graphite002_red`` workspace on the table with ``Ws Index`` equal to ``0-50``.
#. In ``Input file`` choose ``Browse``. Navigate to the ISIS-Sample data folder and select the file ``irs26176_graphite002_red.nxs``. A new entry, ``irs26176_graphite002_red``, should be
   added to the data table, with ``Ws Index`` equal to ``0-50``. (beware that there is a ``irs26176_graphite002_red.nxs`` file on the ``Usage Data`` set. This is NOT the correct file for this test,
   it should be loaded from the Sample Data-ISIS set)
#. With both workspaces on the data table, click on  the ``Select All`` button and then on the ``Add Data`` button. Both workspaces should be added to the ``Elwin`` interface.
#. Change the integration range from -0.2 to 0.2
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name.
#. In the main GUI right-click on ``irs26174-26176_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`ln((meV))^{-1}` vs :math:`Q2`


I(Q, t) tab
-----------

**Time required 3 - 5 minutes for each**

--------------

1. Direct data
##############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Iqt`` tab
#. Load the ``MARI27691_sqw.nxs`` file from the sample data
#. Load the resolution file ``MARI27698_sqw.nxs`` from the sample data
#. Click ``Run``
#. A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a workspace with 17 histograms and 3 bins.
#. Click ``Plot Current preview`` this should plot the same data as the preview window
#. Choose some workspace indices (e.g. 0-2) in the ``Output`` section and click ``Plot Spectra`` this should give a plot with the title *MARI27691_iqt*
#. Click the down arrow on the ``Plot Spectra`` button and then select ``Plot Tiled``. This should give a tiled plot of the selected workspace indices.

.. _iqt_inelastic_test:

2. Indirect data
################

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Processor``
#. Go to the ``Iqt`` tab
#. Load the ``irs26176_graphite002_red.nxs`` file from the sample data
#. Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data
#. Click ``Run``
#. A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a workspace with 51 histograms and 86 bins. **NB** keep this workspace if you are doing the :ref:`QENS Fitting Manual Test <inelastic_qens_fitting_testing>`
#. Click ``Plot Current preview`` this should plot the same data as the preview window
#. Choose some workspace indices (e.g. 0-2) in the ``Output`` section and click ``Plot Spectra`` this should give a plot with the title *irs26176_graphite002_iqt*
#. Click the down arrow on the ``Plot Spectra`` button and then select ``Plot Tiled``. This should give a tiled plot of the selected workspace indices.
