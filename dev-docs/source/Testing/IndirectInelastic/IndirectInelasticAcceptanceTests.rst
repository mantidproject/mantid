.. _indirect_inelastic_testing:

Indirect Inelastic Testing
==========================

.. contents::
   :local:

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
#. Enter ``Run number`` 26173
#. Click ``Run``
#. This should generate a workspace with ``_calib`` at the end, delete this workspace
#. Change the ``Scale by factor`` value to 0.5
#. Click ``Run``
#. This should generate a workspace with ``_calib`` at the end
#. Check the ``Create RES`` box
#. Click ``Run``
#. This should generate a workspace with ``_res`` at the end
#. Click ``Plot result`` - should produce two plots
#. Enter ``Run number`` 55878-55879 and check ``Sum Files``
#. Click ``Run``, this should produce a ``_calib`` workspace
#. Make sure that you keep the ``_calib`` workspace, it is needed for the next test
#. Enter ``Run number`` 59057-59059 and check ``Sum Files``
#. Set ``Reflection`` to 004
#. This should produce a new ``_calib`` workspace, with ``004`` in the name

**Time required 5-10 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
#. Make sure ``Instrument`` is set to ``IRIS``
#. Check the ``Sum Files`` box
#. In the ``Run files`` box enter ``26184-26185``
#. Click ``Run``
#. Check the ``Use Calib File`` box
#. Change ``File`` to ``Workspace`` and choose the ``_calib`` workspace previously created (From the previous test)
#. Click ``Run``
#. In the main GUI right-click on the ``iris26184_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 51
#. Click ``Cancel``
#. In the ``Data reduction`` GUI, change the setting ``Mode`` to Groups
#. Set ``Groups`` to 5
#. Click ``Run``
#. In the main GUI right-click on the ``iris26184_multi_graphite002_red`` workspace
#. Choose ``Plot spectrum``, note the number of spectra, should be 6
#. Choose ``Plot All``, this should result in a plot of all 6 spectra
#. Go to the ``S(Q, W)`` tab
#. Change ``File`` to ``Workspace`` and load the ``_red`` workspace just created
#. Set ``Q-Low`` - ``Q-width`` - ``Q-High`` to 0.2, 0.2, 1.8
#. Click ``Run``
#. Should create a ``_sqw`` workspace
#. Check ``Rebin in energy``
#. Set ``E-Low`` - ``E-width`` - ``E-High`` to -0.5, 0.01, 0.5
#. Click ``Run``
#. With ``Plot Output`` set as Contour click ``Plot``, this should result in a 2D contour plot
#. Click ``Manage User Directories`` and set the default save location
#. Click ``Save Result``
#. Check that the ``.nxs`` file was created in the correct location

Data analysis Elwin
-------------------

*Preparation*

-  ISIS Sample data set, `available here <http://download.mantidproject.org/>`_
-  Make sure that the data set is on your list of search directories

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
#. Enter ``irs26176_graphite002_red.nxs`` in ``Input file``
#. Click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Now in ``Input file`` choose browse, navigate to the ISIS-Sample data and select the two files above simultaneously, by using shift key
#. Change the integration range from -0.2 to 0.2
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name
#. In the main GUI right-click on ``irs26174-26176_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`A^2` vs :math:`Q`
#. Right-click on the ``irs26176_graphite002_elwin_eq`` workspace and ``Save Nexus``; save to a location of your choice; you will use this file in the next test

Data analysis MSD
-----------------

*Preparation*

-  The ``_eq.nxs`` file from the previous test

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
#. Go to the ``MSD fit`` tab
#. Load the file that you saved in the previous test
#. Set ``Fit type`` to Gaussian
#. Click ``Run``
#. This should produce a plot of the fitted function in the interface
#. Change ``End X`` to 1.0
#. Click ``Run``
#. Repeat the previous steps with ``Peters`` and ``Yi`` functions
#. Try run fits using the different ``Minimizer`` options (except FABADA), each time change the ``End X`` value either + or - 0.1

Data analysis Conv Fit
----------------------

*Preparation*

-  ISIS Sample data set, `available here <http://download.mantidproject.org/>`_

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
#. Go to the ``Conv Fit`` tab
#. Load the ``irs26176_graphite002_red.nxs`` file from the sample data
#. Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data
#. Set ``Fit spectra`` to 0 - 5
#. Set ``Fit type`` to Two Lorentzians
#. Set ``Max iterations`` to 400
#. Click ``Run``
#. Three new workspaces should be created in the main GUI - ``Parameters``, ``Result`` and ``Workspaces``
#. In the ``Fit`` tab, change ``Fit spectra`` to String and enter 3
#. Click ``Run`` the plot should update and new workspaces are created in the main Mantid GUI
#. Set ``Fit spectra`` to String 3
#. Click ``Run``; the plot should update and new workspaces are created in the main Mantid GUI
#. Try the various ``Plot`` options in the interface

   (a)  ``Plot Output`` set to All and click plot - should give 4 separate plots
   (b)  ``Plot Current Preview`` - should result in a plot with three datasets
   (c)  Enable the ``Plot Guess`` checkbox - should not change anything, but should not break anything either!

#. Change the ``Fit type`` to different functions and run fits

Data analysis I(Q, T)
----------------------

*Preparation*

-  Access to ISIS sample data

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
#. Go to the ``I(Q, T)`` tab
#. Load the ``irs26176_graphite002_red.nxs`` file from the sample data
#. Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data 
#. Click ``Run``
#. A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a 87 x 6 table. **NB** keep this workspace for the next test
#. Click ``Plot Current View`` this should plot the same data as the preview window
#. Click ``Plot Spectrum`` this should give a plot with the title *irs26176_graphite002_iqt*   
#. Click ``Plot Tiled`` this should give a tiled plot of spectra between the range specified.

Data analysis I(Q, T) Fit
-------------------------

*Preparation*

-  The ``_iqt`` workspace from the previous test

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
#. Go to the ``I(Q, T) Fit`` tab
#. Load the ``_iqt`` workspace from the previous test
#. Set ``Exponential`` to 1
#. Set ``EndX`` to 0.14
#. Click ``Run``
#. This should produce a fit and a difference plot in the window
#. Click ``Plot current preview`` this should open a plot with three datasets plotted
#. Change ``Range`` to 0 - 5
#. Click ``Run``
#. Select Lifetime from the ``Plot Output`` drop-down
#. Click ``Plot`` this should open a new plot with the lifetimes plotted
  
