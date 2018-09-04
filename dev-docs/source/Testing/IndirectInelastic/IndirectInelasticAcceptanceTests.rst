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

1.  Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
2.  Make sure ``Instrument`` is set to ``IRIS``
3.  Go to the ``ISIS calibration``
4.  Enter ``Run number`` 26173
5.  Click ``Run``
6.  This should generate a workspace with ``_calib`` at the end . Delete
    this workspace
7.  Change the ``Scale by factor`` value to 0.5
8.  Click ``Run``
9.  This should generate a workspace with ``_calib`` at the end
10. Check the ``Create RES`` box
11. Click ``Run``
12. This should generate a workspace with ``_res`` at the end
13. Click ``Plot result`` - should produce two plots
14. Enter ``Run number`` 55878-55879 and check ``Sum Files``
15. Click ``Run``, this should produce a ``_calib`` workspace
16. Make sure that you keep the ``_calib`` workspace, it is needed for
    the next test
17. Enter ``Run number`` 59057-59059 and check ``Sum Files``
18. Set ``Reflection`` to 004
19. This should produce a new ``_calib`` workspace, with ``004`` in the
    name

**Time required 5-10 minutes**

--------------

1.  Open ``Interfaces`` > ``Indirect`` > ``Data reduction``
2.  Make sure ``Instrument`` is set to ``IRIS``
3.  Check the ``Sum spectra`` box
4.  In the ``Run files`` box enter ``26184-26185``
5.  Click ``Run``
6.  Check the ``Use Calib File`` box
7.  Change ``File`` to ``Workspace`` and choose the ``_calib`` workspace
    previously created (From the previous test)
8.  Click ``Run``
9.  In the main GUI right-click on the
    ``iris26184_multi_graphite002_red`` workspace
10. Choose ``Plot spectrum``, note the number of spectra, should be 50
11. Click ``Cancel``
12. In the ``Data reduction`` GUI, change the setting ``Mode`` to Groups
13. Set ``Groups`` to 5
14. Click ``Run``
15. In the main GUI right-click on the
    ``iris26184_multi_graphite002_red`` workspace
16. Choose ``Plot spectrum``, note the number of spectra, should be 6
17. Choose ``Plot All``
18. Go to the ``S(Q, W)`` tab
19. Change ``File`` to ``Workspace`` and load the ``_red`` workspace
    just created
20. Set ``Q-Low``-``Q-width``-``Q-High`` to 0.2, 0.2, 1.8
21. Click ``Run``
22. Should create a ``_sqw`` workspace
23. Check ``Rebin in energy``
24. Set ``Q-Low``-``Q-width``-``Q-High`` to -0.5, 0.01, 0.5
25. Click ``Run``
26. With ``Plot Output`` set as Contour click ``Plot``
27. Click ``Save Result``

Data analysis Elwin
-------------------

*Preparation*

-  ISIS Sample data set
-  Make sure that the data set is on your list of search directories

**Time required 3 - 5 minutes**

--------------

1.  Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
2.  Enter ``irs26176_graphite002_red.nxs`` in ``Input file``
3.  Click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
4.  Now in ``Input file`` choose browse, navigate to the ISIS-Sample data and select the two files above simultaneously, by using shift key
5.  Change the integration range from -0.2 to 0.2
6.  Click ``Run``
7.  This should result in three new workspaces again, this time with file ranges as their name
8.  In the main GUI right-click on ``irs26174-26176_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
9.  This should plot two lines of :math:`A^{-2}` vs :math:`Q`
10.  Right-click on the ``irs26176_graphite002_elwin_eq`` workspace and ``Save Nexus``; save to a location of your choice

Data analysis MSD
-----------------

*Preparation*

-  The ``_eq.nxs`` file from the previous test

**Time required 3 - 5 minutes**

--------------

1.  Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
2.  Go to the ``MSD fit`` tab
3.  Load the file that you saved in the previous test
4.  Set ``Fit type`` to Gaussian
5.  Click ``Run``
6.  This should produce a plot of the fitted function in the interface
7.  Change ``End X`` to 1.0
8.  Click ``Run``
9.  Repeat the previous steps with ``Peters`` and ``Yi`` functions
10.  Try run fits using the different ``Minimizer`` options (except FABDA), each time change the ``End X`` value either + or - 0.1

Data analysis Conv Fit
----------------------

*Preparation*

-  Access to ISIS sample data

**Time required 3 - 5 minutes**

--------------

1.  Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
2.  Go to the ``Conv Fit`` tab
3.  Load the ``irs26176_graphite002_red.nxs`` file from the sample data
4.  Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data
5.  Set ``Fit spectra`` to 0 - 50
6.  Set ``Fit type`` to Two Lorentzians
7.  Set ``Max iterations`` to 400
8.  Click ``Run``
9.  Three new workspaces should be greater in the main GUI - ``Parameters``, ``Result`` and ``Workspaces``
10.  In the ``Fit`` tab, change ``Fit spectra`` to String and enter 3
11.  Click ``Run`` the plot should update and new workspaces are created in the main Mantid GUI
12.  Set ``Fit spectra`` to String 3
13.  Click ``Run``; the plot should update and new workspaces are created in the main Mantid GUI
14.  Try the various ``Plot`` options in the interface

   (a)  ``Plot Output``
   (b)  ``Plot Current Preview``
   (c)  Enable the ``Plot Guess`` checkbox

15.  Change the ``Fit type`` to different functions and run fits

Data analysis I(Q, T)
----------------------

*Preparation*

-  Access to ISIS sample data

**Time required 3 - 5 minutes**

--------------

1.  Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
2.  Go to the ``I(Q, T)`` tab
3.  Load the ``irs26176_graphite002_red.nxs`` file from the sample data
4.  Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data 
5.  Click ``Run``
6.  A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a 87 x 51 table. **NB** keep thiw workspace for the next test
7. Click ``Plot Currnet View`` this should plot the same data as the preview window
8. Click ``Plot Result`` this should give a plot with the title *irs26176_graphite002_iqt*   

Data analysis I(Q, T) Fit
-------------------------

*Preparation*

-  The ``_iqt`` workspace from the previous test

**Time required 3 - 5 minutes**

--------------

1.  Go to ``Interfaces`` > ``Indirect`` > ``Data Analysis``
2.  Go to the ``I(Q, T) Fit`` tab
3.  Load the ``_iqt`` workspace from the previous test
  


