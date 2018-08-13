========================
Direct Inelastic Changes
========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.


Interfaces
----------


New features
############

- Added a progress bar to the bottom of the reduction dialog indicating the progress of the currently running reduction.
- Added the ability to save the results of the TOFTOF reduction as Ascii files.


Improvements
############

- Improved ``Save``-section of the TOFTOF reduction dialog.
- Behavior of the :ref:`LoadDNSLegacy <algm-LoadDNSLegacy>` for TOF data has been changed: the algorithm does not try to guess elastic channel any more, but asks for the user input.

:ref:`Release 3.14.0 <v3.14.0>`


Improvements
############

- :ref:`LoadDNSSCD <algm-LoadDNSSCD>` has been improved to be able to load TOF data.
