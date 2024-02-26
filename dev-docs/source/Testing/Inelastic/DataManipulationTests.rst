.. _inelastic_data_manipulation_testing:

Inelastic Data Manipulation Testing
===================================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_
- Make sure that the data set is on your list of search directories

Elwin tab
---------

**Time required 3 - 5 minutes for each**

--------------

1. Direct data
##############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
#. Go to the ``Elwin`` tab
#. Enter ``MAR27691_red.nxs`` in ``Input file``
#. Click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Now in ``Input file`` choose browse, navigate to the ISIS-Sample data and select the two files, ``MAR27691_red.nxs`` and ``MAR27698_red.nxs``, by using shift key
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name
#. In the main GUI right-click on ``MAR27691-27698_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`ln(Counts(microAmp.hour))^{-1}` vs :math:`Q2`

.. _elwin_inelastic_test:

2. Indirect data
################

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
#. Go to the ``Elwin`` tab
#. Enter ``irs26176_graphite002_red.nxs`` in ``Input file``
#. Click ``Run`` - this should produce 3 new workspaces ``_elf``, ``_eq`` and ``_eq2``
#. Now in ``Input file`` choose browse, navigate to the ISIS-Sample data and select ``irs26176_graphite002_red.nxs`` and ``irs26174_graphite002_red.nxs`` simultaneously, by using shift key
#. Change the integration range from -0.2 to 0.2
#. Click ``Run``
#. This should result in three new workspaces again, this time with file ranges as their name
#. In the main GUI right-click on ``irs26174-26176_graphite002_red_elwin_eq2`` and choose ``Plot Spectrum``, choose ``Plot All``
#. This should plot two lines of :math:`ln((meV))^{-1}` vs :math:`Q2`
#. Right-click on the ``irs26176_graphite002_elwin_eq`` workspace and ``Save Nexus``; save to a location of your choice; you will use this file in the next test

I(Q, T) tab
-----------

**Time required 3 - 5 minutes for each**

--------------

1. Direct data
##############

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
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

#. Go to ``Interfaces`` > ``Inelastic`` > ``Data Manipulation``
#. Go to the ``Iqt`` tab
#. Load the ``irs26176_graphite002_red.nxs`` file from the sample data
#. Load the resolution file ``irs26173_graphite002_res.nxs`` from the sample data
#. Click ``Run``
#. A new workspace with the suffix ``_iqt`` should appear in the main GUI, it should be a workspace with 51 histograms and 86 bins. **NB** keep this workspace for the next test
#. Click ``Plot Current preview`` this should plot the same data as the preview window
#. Choose some workspace indices (e.g. 0-2) in the ``Output`` section and click ``Plot Spectra`` this should give a plot with the title *irs26176_graphite002_iqt*
#. Click the down arrow on the ``Plot Spectra`` button and then select ``Plot Tiled``. This should give a tiled plot of the selected workspace indices.
