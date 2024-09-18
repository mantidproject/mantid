.. _indirect_data_reduction_testing:

Indirect Data Reduction Testing
===============================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_

ISIS Calibration
----------------

*Preparation*

-  Instrument ``IRIS``
-  You need access to the ISIS data archive

**Time required 2-5 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
#. Make sure ``Instrument`` is set to ``IRIS``
#. Go to the ``ISIS calibration``
#. Enter ``Input Runs`` 26173
#. Click ``Run``
#. This should generate a workspace with ``_calib`` at the end, delete this workspace
#. Check the ``Scale by factor`` checkbox and change the value of the scale to 0.5.
#. Click ``Run``
#. This should generate a workspace with ``_calib`` at the end
#. Make sure to keep the ``_calib`` workspace, it is needed for the :ref:`ISIS Diagnostics<isis_diagnostic_test>` test
#. Check the ``Create RES`` box
#. Click ``Run``
#. This should generate a workspace with ``_res`` at the end
#. In the ``Output`` options, select the ``_res`` workspace and click ``Plot Spectra``. This should produce a spectrum plot.
#. Then select the ``_calib`` workspace and use the down arrow to click ``Plot Bins``. This should produce a bin plot.
#. Enter ``Input Runs`` 55878-55879 and check ``Sum Files``
#. Click ``Run``, this should produce a ``_calib`` workspace
#. Make sure to also keep this ``_calib`` workspace, it is needed for the next test
#. Enter ``Input Runs`` 59057-59059 and check ``Sum Files``
#. Set ``Reflection`` to 004
#. Click ``Run``
#. This should produce a new ``_calib`` workspace, with ``004`` in the name.
#. Before moving on, set ``Reflection`` to 002

ISIS Energy Transfer
--------------------
**Time required 5-10 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
#. Make sure ``Instrument`` is set to ``IRIS``
#. Make sure the tab is set to ``ISIS Energy Transfer``
#. Check the ``Sum Files`` box
#. In the ``Input Runs`` box enter ``26184-26185``
#. Click ``Run``
#. Check the ``Use Calib File`` box
#. Change ``File`` to ``Workspace`` and choose the ``_calib`` workspace previously created (55878 from the previous test)
#. Click ``Run``
#. In the main GUI right-click on the ``iris26184-26185_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 51
#. Click ``Cancel``
#. In the ``Data reduction`` GUI, change the ``Detector Grouping`` to Groups
#. Set ``Groups`` to 5
#. Click ``Run``
#. In the main GUI right-click on the ``iris26184-26185_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 6
#. Choose ``Plot All``, this should result in a plot of all 6 spectra
#. Open ``Interfaces`` > ``Inelastic`` > ``Data Processor`` and go to the ``S(Q, W)`` tab
#. Change ``File`` to ``Workspace`` and load the ``_red`` workspace just created
#. ``Q-Low`` and ``Q-High`` should be automatically updated to the y axis range of the contour plot.
#. ``E-Low`` and ``E-High`` should be automatically updated to the x axis range of the contour plot.
#. Click ``Run``. An ``_sqw`` workspace should be created.
#. Check ``Rebin in energy``
#. Click ``Run``
#. Within the ``Output`` section, click the down arrow on the ``Plot Spectra`` button and then select ``Open Slice Viewer``, this should open a slice viewer window.
#. Click ``Manage User Directories`` and set the default save location
#. Click ``Save Result``
#. Check that the ``.nxs`` file was created in the correct location

.. _isis_diagnostic_test:

ISIS Diagnostics
----------------
**Time required 5-10 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
#. With ``Instrument`` set to ``IRIS``, navigate to the ``ISIS Diagnostic`` tab
#. On ``Input Runs``, type ``26176`` and press the Enter key
#. The plot widget should render a spectrum with two black sliders indicating integration limits
#. On the property browser, change the value of the  ``Preview Spectrum`` property, the plot should update with the appropriate spectrum
   whenever the number is changed, unless is outside the range of allowed spectra
#. Move the integration sliders, and check that the values of ``Start`` and ``End`` under the ``Peak`` property on the property browser are accordingly updated
#. Change the values of ``Start`` and ``End`` on the property browser and check that the slider position is updated on the plot
#. Tick the ``Use Calibration`` checkbox and select the workspace previously generated, ``irs26173_graphite002_calib``, on the data selector
#. Change the ``Spectra Min`` property to 3
#. Click ``Run``
#. A preview spectra should be rendered, plotting the integrated counts versus the spectrum number.
#. A new workspace with suffix ``_slice`` should be generated on the ADS
#. On Output, clicking the button ``Plot Spectra`` should open a new plot window with the same data as the preview plot.
#. Tick on the ``Use Two Ranges`` property. Two green sliders should appear on the plot
#. Move the green sliders and check that the ``Start`` and ``End`` properties under the
   ``Background`` property are updated accordingly
#. Selecting a non-overlapping background range, click on the ``Run`` button
#. The preview plot and the workspace ending in ``_slice`` should update with the new integrated time slice

Transmission
-------------

**Time required 3-5 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
#. Make sure ``Instrument`` is set to ``IRIS``, ``Analyser`` to ``graphite`` and ``Reflection`` to ``002``
#. Navigate to the ``Transmission`` tab
#. On ``Sample``, type ``26176`` and press the Enter key
#. On ``Background``, type ``26174`` and press the Enter key
#. Click ``Run``
#. The preview plot should be rendered, displaying three lines indicating the monitor detection for the can, the sample, and the transmission
#. A workspace with suffix `_transmission`, and a workspace group with suffix `_transmission_group` should be generated on the ADS. The `_transmission_group` workspace
   should contain three workspaces with suffices `_Sam`, `_Can` and `_Trans`
#. Clicking the ``Plot Spectra`` button, a plot window should prompt, displaying the same lines as in the preview plot



