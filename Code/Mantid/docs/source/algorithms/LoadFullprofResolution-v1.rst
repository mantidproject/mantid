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

Usage
-----

** Example - Run LoadprofResolution for both TableWorkspace and workspace with XX Instrument **

.. testcode:: ExLoadFullprofResolutionSimple

   # We run LoadFullprof Resolution with both the OutputTable workspace
   # and an appropriate output workspace (group of 2)
   # selecting 2 banks from the IRF file
   ws = Load("MUSR00015189")

   tws = LoadFullprofResolution("MUSR_01.irf",Banks="3,5", Workspace="ws")

   #Print first four rows
   for i in [0,1,2,3]:
      row = tws.row(i)
      print row

Output:

.. testoutput:: ExLoadFullprofResolutionSimple

   {'Value_5': 5.0, 'Name': 'BANK', 'Value_3': 3.0}
   {'Value_5': 1.607107, 'Name': 'Alph0', 'Value_3': 1.597107}
   {'Value_5': 1.296805, 'Name': 'Alph1', 'Value_3': 1.496805}
   {'Value_5': 37.567333, 'Name': 'Beta0', 'Value_3': 33.567333}

.. categories::
