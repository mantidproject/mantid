.. _inelastic_bayes_fitting_testing:

Inelastic Bayes Fitting Testing
===============================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Usage Data <http://download.mantidproject.org>`_

ResNorm tab
-----------

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Bayes fitting``
#. Go to the ``ResNorm`` tab
#. With the vanadium combo box set to ``File`` click browse and select the file ``irs26173_graphite002_red`` from the Usage Data folder
#. With the resolution combo box set to ``File`` click browse and select the file ``irs26173_graphite002_res`` from the Usage Data folder
#. Click ``Run``
#. This should produce two workspaces with ``ResNorm`` and ``ResNorm_Fit`` suffixes
#. Make sure that moving the black sliders in the plot will change the value of ``EMin`` and ``EMax``
#. Change the ``Preview Spectrum`` value and the plot should be updated with the corresponding workspace index
#. Check that the workspace index maximum is 9 (the last workspace index in the vanadium workspace)
#. Check that when clicking ``Plot Current Preview``, it will open a pop-up window with the current preview plot
#. In the ``Output Options``, select ``All`` and click ``Plot``, it should open two plots suffixed with ``Intensity`` and ``Stretch``
#. Plot individual results by selecting other options, it should only open one plot with the selected result

Quasi Tab
---------

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Bayes fitting``
#. Go to the ``Quasi`` tab
#. Click on ``Manage Directories`` and set the default save directory to any location in your machine
#. With the vanadium combo box set to ``File`` click browse and select the file ``irs26176_graphite002_red`` from the Usage Data folder
#. With the resolution combo box set to ``File`` click browse and select the file ``irs26173_graphite002_res`` from the Usage Data folder
#. Click ``Run``
#. This should produce three workspaces with ``Fit``, ``Prob`` and ``Result`` suffixes
#. Make sure that moving the black sliders in the plot will change the value of ``EMin`` and ``EMax``
#. Run the tab with different options in the ``Fit Options`` section
#. Check that the workspace index maximum is 9 (the last workspace index in the vanadium workspace)
#. Check that when clicking ``Plot Current Preview``, it will open a pop-up window with the current preview plot
#. In the ``Output Options``, select ``All`` and click ``Plot``, it should open three plots with ``Prob``, ``Amplitude`` and ``FWHM`` data
#. Plot individual results by selecting other options, it should only open one plot with the selected attribute

Stretch Tab
-----------

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Bayes fitting``
#. Go to the ``Stretch`` tab
#. Click on ``Manage Directories`` and set the default save directory to any location in your machine
#. With the vanadium combo box set to ``File`` click browse and select the file ``irs26176_graphite002_red`` from the Usage Data folder
#. With the resolution combo box set to ``File`` click browse and select the file ``irs26173_graphite002_res`` from the Usage Data folder
#. Click ``Run``
#. This should produce two workspaces with ``Stretch_Contour`` and ``Stretch_Fit`` suffixes
#. Make sure that moving the black sliders in the plot will change the value of ``EMin`` and ``EMax``
#. Run the tab with different settings in the ``Fit Options`` section
#. Check that the workspace index maximum is 9 (the last workspace index in the vanadium workspace)
#. Check that when clicking ``Plot Current Preview``, it will open a pop-up window with the current preview plot
#. In the ``Output Options``, select ``All`` and click ``Plot``, it should open three plots with ``Sigma`` and ``Beta`` data
#. Plot individual results by selecting other options, it should only open one plot with the selected attribute
#. Click ``Plot Contour`` to plot the stretch contour workspace