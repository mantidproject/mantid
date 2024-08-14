.. _inelastic_qens_fitting_testing:

Inelastic QENS Fitting Testing
==============================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_
- Make sure that the data set is on your list of search directories

MSD tab
-------

*Preparation*

-  The ``_eq.nxs`` file from the :ref:`Elwin Indirect Inelastic <elwin_inelastic_test>` test

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``QENS fitting``
#. Go to the ``MSD`` tab
#. Click ``Add Workspace``
#. With the combo box set to ``File`` click browse and select the file that you saved in the previous test
#. Check ``All Spectra``
#. Click ``Add`` and close the dialogue.
#. Set ``Fit type`` to Gaussian
#. Click ``Run``
#. This should produce a plot of the fitted function in the interface
#. Change ``End X`` to 1.0
#. Click ``Run``
#. Repeat the previous steps with ``Peters`` and ``Yi`` functions
#. Try run fits using the different ``Minimizer`` options (except FABADA), each time change the ``End X`` value either + or - 0.1

I(Q, T) tab
-----------

*Preparation*

-  The ``_iqt`` workspace from the :ref:`I(Q, T) Indirect Inelastic <iqt_inelastic_test>` test

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``QENS fitting``
#. Go to the ``I(Q, T)`` tab
#. Click ``Add Workspace``
#. With the combo box set to ``Workspace`` select the ``_iqt`` workspace from the previous test
#. Check ``All Spectra``
#. Click ``Add`` and close the dialogue.
#. Set ``Exponential`` to 1
#. In the data table set ``EndX`` for WS Index 0 to 0.14
#. Using shift select the ``EndX`` for all spectra and click unify range, this should set the ``EndX`` for all spectra to 0.14
#. Click ``Run``
#. This should produce a fit and a difference plot in the window
#. Click ``Plot current preview`` this should open a plot with three datasets plotted
#. Select the runs 6 - 50 and click ``Remove``
#. Click ``Run``
#. Select Lifetime from the ``Output`` drop-down
#. Click ``Plot`` this should open a new plot with the lifetimes plotted

.. _convolution_inelastic_test:

Convolution tab
---------------

*Preparation*

-  ISIS Sample data set, `available here <http://download.mantidproject.org/>`_

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``QENS fitting``
#. Go to the ``Convolution`` tab
#. Click ``Add Workspace``
#. With the combo box's set to ``File``
#. Click browse and load the ``irs26176_graphite002_red.nxs`` file from the sample data
#. Click browse and load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data
#. In Workspace Indices enter ``0-5``
#. Click ``Add`` and close the dialogue.
#. Set ``Lorentzians`` to 2
#. Set ``Max iterations`` to 400
#. Click ``Run``
#. Three new workspaces should be created in the main GUI - ``Parameters``, ``Result`` and ``Workspaces``
#. In the ``Mini plots`` widget, change ``Plot Spectrum`` to 3
#. Click ``Fit Single Spectrum`` the plot should update and new workspaces are created in the main Mantid GUI
#. Remove spectra 0-2, then remove spectra 5. do these as separate removals.
#. Click ``Run``; the plot should update and new workspaces are created in the main Mantid GUI
#. Try the various ``Plot`` options in the interface

   (a)  ``Output`` drop-down set to All and click ``Plot`` - should give 5 separate plots
   (b)  ``Plot Current Preview`` - should result in a plot with three datasets
   (c)  Enable the ``Plot Guess`` checkbox - should not change anything, but should not break anything either!

#. Change the ``Fit type`` to different functions and run fits

Function (Q) tab
----------------

*Preparation*

-  The ``_Result`` workspace output from the :ref:`Convolution tab <convolution_inelastic_test>` test

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``QENS fitting``
#. Go to the ``Function (Q)`` tab
#. Click ``Add Workspace``
#. With the combo box set to ``Workspace`` select the ``0-5__Result`` workspace from the previous test
#. In Parameter Name select ``f1.f0.FWHM``
#. Click ``Add`` and close the dialogue.
#. Under ``Fit Type`` select ``TeixeiraWater``
#. Click ``Run``
#. Three new workspaces should be created in the main GUI - ``Parameters``, ``Result`` and ``Workspaces``
#. Change the ``Fit type`` to different functions and run fits
