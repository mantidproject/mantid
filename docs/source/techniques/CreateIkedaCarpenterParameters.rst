.. _CreateIkedaCarpenterParameters:

Create Ikeda Carpenter Parameters
=================================

.. contents:: Table of Contents
  :local:

Example of how a parameter can be determined on a beamline
----------------------------------------------------------

The steps involved in creating instrument specific parameters for the :ref:`Ikeda-Carpenter-Pseudo-Voigt <func-IkedaCarpenterPV>` peak shape function are on ISIS/GEM approximately as follows:

- Run calibration samples: NIST standard Silicon and Y2O3.
- Combined rietveld refined of these two standards in either Fullprof or GSAS
- All structural parameters are fixed but not the Ikeda Carpenter parameters and background parameters (and zero shifts).
  On GEM, the Ikeda-Carpenter parameters are the same for all banks but the Pseudo-Voigt sigma and gamma parameters which are refined differently for the different banks.
- Finally the information from this refinement is copied and pasted from output files to generate either a :ref:`Fullprof <CreateIkedaCarpenterParametersFullprof>` or :ref:`GSAS <CreateIkedaCarpenterParametersGSAS>` instrument parameter files.

.. categories:: Techniques
