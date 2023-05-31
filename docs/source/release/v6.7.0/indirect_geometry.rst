=========================
Indirect Geometry Changes
=========================

.. contents:: Table of Contents
   :local:

New Features
------------
- Add option for negative reflection to the indirect data manipulation :ref:`Symmetrise tab <inelastic-symmetrise>`.
- Add Q-Spacing Output for OSIRIS Instrument in :ref:`Indirect Diffraction UI <interface-indirect-diffraction>`. The interface now produces three outputs in three units (Q-spacing, D-spacing, and Tof) instead of the previous two (D-spacing and Tof).
- Add unit option to :ref:`Indirect Diffraction UI <interface-indirect-diffraction>` output plot options. Users can now choose from "D-spacing" or "Q-spacing" unit options.


Bugfixes
--------
- A bug has been fixed that caused the :ref:`Elwin <elwin>` tab to not run specific spectra when using the workspace input method.
- Fixed a hard crash caused by attempting to load an empty list of enabled SD detectors of IN16B data in :ref:`LoadILLIndirect <algm-LoadILLIndirect>`.
- Fixed a bug that meant when the workspace attribute of a function was changed, (e.g. resolution or tabulated function) the function was not updated. This would lead to a crash as Mantid believed that the option was invalid.
- Fixed a hard crash caused by highlighting more cells than rows in the data input table and then pressing ``Remove``.


Algorithms
----------

New features
############
- Major changes have been made to the treatment of temperature in :ref:`Abins <algm-Abins>` and :ref:`Abins2D <algm-Abins2D>`. This gives slightly different results at low temperature (i.e. the 10K default) and significantly different results at higher temperatures. The new methodology is in clearer agreement with the published literature, and gives close results to other implementations. The :ref:`supporting documentation <DynamicalStructureFactorFromAbInitio>` has been updated to reflect the mathematics behind recent versions of Abins/2D.
- Added algorithms for the ANSTO EMU instrument to support :ref:`elastic <algm-ElasticEMUauReduction>` and :ref:`inelastic <algm-InelasticEMUauReduction>` reduction processing.

Bugfixes
############
- Updated the CRYSTAL parser for Abins to accept files with complex eigenvectors expressed as "ANTI-PHASE". This seems to be the notation used for calculations on a regular q-point mesh, such as when performing Fourier interpolation to obtain a dense DOS.
- Fixed a bug when loading certain ``vasprun.xml`` files in Abins. If the VASP user enables ``selective dynamics`` and ``IBRION=6``, VASP will ignore the frozen atoms when calculation vibrational frequencies. This created an unexpected number of degrees of freedom.

:ref:`Release 6.7.0 <v6.7.0>`