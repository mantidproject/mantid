Indirect Data Reduction
=======================

.. contents:: Table of Contents
  :local:

Overview
--------

The Indirect Data Reduction interface provides the initial reduction that
is used to convert raw instrument data to S(Q, w) for analysis in the
Indirect Data Analysis and Indirect Bayes interfaces.

.. interface:: Data Reduction
  :align: right
  :width: 350

.. warning:: Currently this interface only supports ISIS instruments.

Instrument Options
~~~~~~~~~~~~~~~~~~

Instrument
  Used to select the instrument on which the data being reduced was created on

Analyser
  The analyser bank that was active when the experiment was run, or for which
  you are interested in seeing the results of.

Reflection
  The reflection number of the instrument setup

.. tip:: If you need clarification as to the instrument setup you should use
  please speak to the instrument scientist who dealt with your experiment.

Action Buttons
~~~~~~~~~~~~~~

?
  Opens this help page

Py
  Exports a Python script which will replicate the processing done by the current tab

Run
  Runs the processing configured on the current tab

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search

Energy Transfer
---------------

.. interface:: Data Reduction
  :widget: tabEnergyTransfer

Calibration & Resolution
------------------------

.. interface:: Data Reduction
  :widget: tabCalibration

Diagnostics
-----------

.. interface:: Data Reduction
  :widget: tabTimeSlice

Transmission
------------

.. interface:: Data Reduction
  :widget: tabTransmission

Symmetrise
----------

.. interface:: Data Reduction
  :widget: tabSymmetrise

S(Q, w)
-------

.. interface:: Data Reduction
  :widget: tabSofQW

Moments
-------

.. interface:: Data Reduction
  :widget: tabMoments

.. categories:: Interfaces Indirect
