.. _indirect_inelastic_testing:

Indirect Inelastic Testing
==========================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_

Data reduction
--------------

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
#. Change the ``Scale by factor`` value to 0.5
#. Click ``Run``
#. This should generate a workspace with ``_calib`` at the end
#. Check the ``Create RES`` box
#. Click ``Run``
#. This should generate a workspace with ``_res`` at the end
#. In the ``Output`` options, select the ``_res`` workspace and click ``Plot Spectra``. This should produce a spectrum plot.
#. Then select the ``_calib`` workspace and use the down arrow to click ``Plot Bins``. This should produce a bin plot.
#. Enter ``Input Runs`` 55878-55879 and check ``Sum Files``
#. Click ``Run``, this should produce a ``_calib`` workspace
#. Make sure that you keep the ``_calib`` workspace, it is needed for the next test
#. Enter ``Input Runs`` 59057-59059 and check ``Sum Files``
#. Set ``Reflection`` to 004
#. This should produce a new ``_calib`` workspace, with ``004`` in the name.
#. Before moving on, set ``Reflection`` to 002

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
#. In the main GUI right-click on the ``iris26184_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 51
#. Click ``Cancel``
#. In the ``Data reduction`` GUI, change the ``Detector Grouping`` to Groups
#. Set ``Groups`` to 5
#. Click ``Run``
#. In the main GUI right-click on the ``iris26184_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 6
#. Choose ``Plot All``, this should result in a plot of all 6 spectra
#. Open ``Interfaces`` > ``Inelastic`` > ``Data Manipulation`` and go to the ``S(Q, W)`` tab
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
