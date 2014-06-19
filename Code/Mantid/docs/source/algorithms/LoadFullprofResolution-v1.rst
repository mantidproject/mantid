.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load Fullprof resolution (.irf) file to TableWorkspace(s) and optionally
into the instruments of matrix workspaces with one workspace per bank of
the .irf file. Either or both of the Tableworkspace(s) and matrix
workspace must be set.

Where a Workspace is specified the support for translating Fullprof
resolution parameters into the workspace for subsequent fitting is
limitted to Fullprof:

-  NPROF=13, Ikeda-Carpender pseudo-Voigt translated into
   `IkedaCarpenterPV <IkedaCarpenterPV>`__ according to
   `CreateIkedaCarpenterParameters <CreateIkedaCarpenterParameters>`__
-  NPROF=9, back-to-back-exponential pseudo-Voigt translated into
   `BackToBackExponential <BackToBackExponential>`__ according to
   `CreateBackToBackParameters <CreateBackToBackParameters>`__

Note for NPROF=9 the translation is currently ignoring the Lorentzian
part of the pseudo-Voigt.

.. categories::
