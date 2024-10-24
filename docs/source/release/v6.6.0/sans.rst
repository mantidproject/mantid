============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- Mantid now supports loading D16 old scan data in their new format with the entire scan in one file.
- Multiple wavelength ranges can now be processed in a single call to :ref:`SANSScriptingWavRangeReduction`. This is more efficient than calling the function separately for each range. To do this, pass the start/end values as lists, e.g. for ranges "1-2, 3-4, 5-6" pass ``wav_start=[1, 3, 5]`` and ``wav_end=[2, 4, 6]``.
- Fix the inverted 2 theta axis of the output for data treated using :ref:`algm-SANSILLParameterScan`.
- Added an interface for the simple reduction and exploration of ILL scan data, especially from D16.

Bugfixes
--------
- There can now only be one instance of the GUI open at any time. Selecting the GUI from the ``Interfaces`` menu when there is already one open will now re-show the currently open GUI.
- The invalid Log binning option for 2D reductions has been removed. Loading a ``TOML`` user file with this value set will now give a warning.

:ref:`Release 6.6.0 <v6.6.0>`
