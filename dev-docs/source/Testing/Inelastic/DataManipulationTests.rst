.. _direct_inelastic_testing:

Direct Inelastic Testing
==========================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_

Data analysis Elwin
-------------------

*Preparation*

-  ISIS Sample data set, `available here <http://download.mantidproject.org/>`_
-  Make sure that the data set is on your list of search directories

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
#. Go to the ``Elwin`` tab
#. Enter ``MAR27691_red.nxs`` in ``Input file``
#. Click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Now in ``Input file`` choose browse, navigate to the ISIS-Sample data and select the two files, MAR27691_red.nxs and MAR27698_red.nxs, by using shift key
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name
#. In the main GUI right-click on ``MAR27691-27698_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`ln(Counts(microAmp.hour))^{-1}` vs :math:`Q2`


Data analysis I(Q, T)
----------------------

*Preparation*

-  Access to ISIS sample data

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
#. Go to the ``Iqt`` tab
#. Load the ``MAR27691_sqw.nxs`` file from the sample data
#. Load the resolution file ``MAR27698_sqw.nxs`` from the sample data
#. Click ``Run``
#. A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a workspace with 281 histograms and 38 bins.
#. Click ``Plot Current preview`` this should plot the same data as the preview window
#. Choose some workspace indices (e.g. 0-2) in the ``Output`` section and click ``Plot Spectra`` this should give a plot with the title *MAR27691_iqt*
#. Click the down arrow on the ``Plot Spectra`` button and then select ``Plot Tiled``. This should give a tiled plot of the selected workspace indices.