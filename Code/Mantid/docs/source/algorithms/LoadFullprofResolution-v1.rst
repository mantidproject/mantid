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
   `IkedaCarpenterPV <http://www.mantidproject.org/IkedaCarpenterPV>`_ according to
   `CreateIkedaCarpenterParameters <http://www.mantidproject.org/CreateIkedaCarpenterParameters>`_
-  NPROF=9, back-to-back-exponential pseudo-Voigt translated into
   `BackToBackExponential <http://www.mantidproject.org/BackToBackExponential>`_ according to
   `CreateBackToBackParameters <http://www.mantidproject.org/CreateBackToBackParameters>`_

Note for NPROF=9 the translation is currently ignoring the Lorentzian
part of the pseudo-Voigt.

.. categories::
