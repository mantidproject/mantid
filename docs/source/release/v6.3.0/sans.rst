============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Improvements
------------

- Added missing logs to save in the header of ASCII files by :ref:`DrILL <DrILL-ref>` for D11lr and D22lr ILL instruments.
- ISIS SANS data reduction now converts to wavelength once, instead of n times for each wavelength pair specified. This reduces the runtime by up to 50%.

Bugfixes
--------

- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will now generate and display a range of wavelength bins
  specified in TOML files. This previously showed an empty field, despite correctly using the input from the TOML file.
- Loading a ``CSV`` file with a missing column now shows an error box explaining the problem. Previously, it showed an uncaught exception.


:ref:`Release 6.3.0 <v6.3.0>`
