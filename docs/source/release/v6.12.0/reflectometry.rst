=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- Group workspaces subject to polarization efficiency correction algorithms will be saved with the spin state in ORSO format in the :ref:`algm-SaveISISReflectometryORSO` algorithm.
- New algorithm :ref:`algm-ReflectometryISISCreateTransmission` has been added for creating transmission workspaces that are suitable for passing to :ref:`algm-PolarizationEfficienciesWildes`.

Bugfixes
--------
- Fix a bug in the :ref:`Experiment Settings<refl_exp_instrument_settings>` tab of the :ref:`Reflectometry Interface<interface-isis-refl>` where the input of certain File Paths (such as `"C:"`) in the `Polarization Correction` and `Flood Correction` text edits could cause a crash in Mantid from Windows.
- Fix a bug where :ref:`algm-ReflectometryReductionOneLiveData` was re-writing the spectra map when loading the instrument into the input live data workspace.
- Fix a bug where :ref:`algm-ReflectometryReductionOneLiveData` was not retrieving correctly the exponential correction parameters from the instrument definition file.
- Fix a bug where :ref:`algm-ApplyFloodWorkspace` was modifying spectra for which there is no flood correction data in multiple bin correction files.

:ref:`Release 6.12.0 <v6.12.0>`
