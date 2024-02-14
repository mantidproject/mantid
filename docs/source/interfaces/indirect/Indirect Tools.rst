Indirect Tools
==============

.. contents:: Table of Contents
  :local:

Overview
--------

The Indirect Tools interface contains useful tools that do not fit into the
other interfaces.

.. interface:: Tools
  :width: 350

Action Buttons
~~~~~~~~~~~~~~

Settings
  Opens the :ref:`Settings <inelastic-interface-settings>` GUI which allows you to
  customize the settings for the Indirect interfaces.

?
  Opens this help page.

Manage Directories
  Opens the Manage Directories dialog allowing you to change your search directories
  and default save directory and enable/disable data archive search.

Transmission
------------

The Transmission tab is used to calculate the sample transmission for a given
sample on a given instrument configuration, this can then be used to determine
the sample shape and dimensions prior to an experiment.

Currently this interface supports ISIS and SNS spectrometers.

.. interface:: Tools
  :widget: tabTransmissionCalc

Options
~~~~~~~

Instrument
  Instrument intending to be used.

Analyser
  The analyser bank used for the experiment.

Reflection
  The reflection number of the planned instrument configuration.

Chemical Formula
  The chemical formula of the sample material, in the format used by the
  :ref:`SetSampleMaterial <algm-SetSampleMaterial>` algorithm.

Mass/Number Density
  The density of the sample in :math: `g/cm^3` or :math:`atoms/\mathrm{\AA{}}^3`.

Thickness
  The thickness of the sample in :math:`cm`.

Run
  Runs the processing configured on the current tab.

.. categories:: Interfaces Indirect
