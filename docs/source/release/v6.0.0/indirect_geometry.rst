=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Improvements
############
- Added an option to load diffraction data on IN16B.
- Indirect Data Analysis multiple input tabs can now accept different fit ranges for each spectra.
- :ref:`QENSFitSequential <algm-QENSFitSequential>` now accepts multiple inputs for `StartX` and `EndX` for each spectra.

Bug Fixes
#########
- A bug has been fixed in Indirect data analysis on the Elwin tab that causes the integration range to be reset when changing between plotted workspaces.

:ref:`Release 6.0.0 <v6.0.0>`
