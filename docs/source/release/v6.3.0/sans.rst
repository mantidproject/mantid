============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improvements
------------

- Added missing logs to save in the header of ASCII files by DrILL for D11lr and D22lr ILL instruments
- The ISIS SANS reduction algorithm now converts to wavelength once, instead of n times for each wavelength pair specified. This reduces the runtime by up to 50%.

Bugfixes
--------

- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will now generate and display a range of wavelength bins
  specified in TOML files. This previously showed an empty field, despite correctly using the input from the TOML file.

Bugfixes
--------

- The :ref:`ISIS SANS Interface <ISIS_Sans_interface_contents>` will now generate and display a range of wavelength bins
  specified in TOML files. This previously showed an empty field, despite correctly using the input from the TOML file.

:ref:`Release 6.3.0 <v6.3.0>`
