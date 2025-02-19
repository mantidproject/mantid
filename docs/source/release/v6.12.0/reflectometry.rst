=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

New Features
------------
- :ref:`algm-SaveISISReflectometryORSO` now saves the spin state for group workspaces that are subject to polarisation efficiency correction algorithms.
- New algorithm :ref:`algm-ReflectometryISISCreateTransmission` for creating transmission workspaces that are suitable for passing to :ref:`algm-PolarizationEfficienciesWildes`.

Bugfixes
--------
- ``Polarization Correction`` and ``Flood Correction`` text boxes in the :ref:`Experiment Settings<refl_exp_instrument_settings>` tab of the :ref:`Reflectometry Interface<interface-isis-refl>` are now more robust to file path inputs (e.g. will no longer crash when ``"C:"`` is entered).
- :ref:`algm-ReflectometryReductionOneLiveData` will no longer re-write the spectra map when loading the instrument into the input live data workspace.
- :ref:`algm-ReflectometryReductionOneAuto` now retrieves the correct exponential correction parameters from the instrument definition file.
- :ref:`algm-ApplyFloodWorkspace` will no longer modify spectra for which there is no flood correction data in multiple bin correction files.

:ref:`Release 6.12.0 <v6.12.0>`
