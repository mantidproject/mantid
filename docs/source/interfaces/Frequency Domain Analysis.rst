.. _Frequency_Domain_Analysis_2-ref:

Frequency Domain Analysis
=========================

.. image::  ../images/frequency_domain_analysis_2_home
   :align: right
   :height: 400px

.. contents:: Table of Contents
  :local:

Interface Overview
------------------

This interface is used to analyse the data collected on Muon instruments in the frequency domain. The interface can be
accessed from the main menu of MantidPlot, in *Interfaces → Muon → Frequency Domain Analysis*. For sample
datasets, please visit `Muon - Downloads <http://www.isis.stfc.ac.uk/groups/muons/downloads/downloads4612.html>`_.

Analysis of the frequency spectrum can be achieved by clicking the fit function icon.

Loading
-------

The loading section is used to load data as well as to specify what the current runs of interest are.

**Load Current Run** This will load the latest run for the selected instrument. You must be connected to the
ISIS data archive for this to work. It also currently only works on windows.

**Run selection box** This takes a comma seperated list of run numbers and loads them all in. You may also
specify ranged of runs with a dash. For example ``62260, 62270-3`` will load ``62260, 62270, 62271, 62273``. This also specifies
which runs are used by the rest of the GUI.

**Browse** This lets you browse locally for files to load.

**Clear All** This clears all the data from the current GUI.

**Co-Add** If selected the list of runs chosen are added rather than being loaded individually.

Tabs
----

All these tabs can be dragged away from the interface into their own window.

* :ref:`Home Tab <muon_home_tab-ref>`
* :ref:`Grouping Tab <muon_grouping_tab-ref>`
* :ref:`Phase Table Tab <muon_phase_table_tab-ref>`
* :ref:`Transform Tab <muon_transform_tab-ref>`

Feedback & Comments
-------------------

If you have any questions or comments about this interface or this help page, please
contact the `Mantid team <http://www.mantidproject.org/Contact>`__ or the
`Muon group <http://www.isis.stfc.ac.uk/groups/muons/muons3385.html>`__.

.. categories:: Interfaces Muon