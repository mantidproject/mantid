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
   :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` according to
   `CreateIkedaCarpenterParameters <http://www.mantidproject.org/CreateIkedaCarpenterParameters>`_
-  NPROF=9, back-to-back-exponential pseudo-Voigt translated into
   :ref:`BackToBackExponential <func-BackToBackExponential>` according to
   `CreateBackToBackParameters <http://www.mantidproject.org/CreateBackToBackParameters>`_

Note for NPROF=9 the translation is currently ignoring the Lorentzian
part of the pseudo-Voigt.

Usage
-----

**Example - Run LoadprofResolution for both TableWorkspace and workspace with MUSR Instrument**

.. include:: ../usagedata-note.txt

.. testcode:: ExLoadFullprofResolutionSimple

   ws = Load("MUSR00015189")

   # Run algorithm. MUSR_01.irf has NPROF=13 in 2nd line, so IkedaCarpenterPV will be used.
   tws = LoadFullprofResolution("MUSR_01.irf",Banks="3,5", Workspace="ws")

   #Print first four rows of output table workspace
   print "First 4 rows of OutputTableWorkspace"
   for i in [0,1,2,3]:
      row = tws.row(i)
      print "{'Name': '%s', 'Value_3': %.2f, 'Value_5': %.2f}" % (  row["Name"], row["Value_3"], row["Value_5"] )

   # Get the instrument with the parameters
   inst = ws[0][0].getInstrument()

   # demonstrate that the type of parameters saved are fitting parameters
   print "Type of 3 parameters got from instrument in workspace"
   print "Alpha0 type =", inst.getParameterType('Alpha0')
   print "Beta0 type =", inst.getParameterType('Beta0')
   print "SigmaSquared type =", inst.getParameterType('SigmaSquared')

   # As of the time of writing, 
   # fitting instrument parameters cannot be 
   # accessed through the python API.  
   # They can be accessed via the file in the lext line, if uncommented:
   #SaveParameterFile(ws[0][0], "instParam.xml")

   #This file should contain the lines shown next and similar for other parameters:
   #		<parameter name="Alpha0">
   #			<value val="0 , IkedaCarpenterPV , Alpha0 ,  ,  ,  , Alpha0= , 1.5971070000000001 ,  , TOF , linear ; TOF ; TOF"/>
   #		</parameter>


Output:

.. testoutput:: ExLoadFullprofResolutionSimple

   First 4 rows of OutputTableWorkspace
   {'Name': 'BANK', 'Value_3': 3.00, 'Value_5': 5.00}
   {'Name': 'Alph0', 'Value_3': 1.60, 'Value_5': 1.61}
   {'Name': 'Alph1', 'Value_3': 1.50, 'Value_5': 1.30}
   {'Name': 'Beta0', 'Value_3': 33.57, 'Value_5': 37.57}
   Type of 3 parameters got from instrument in workspace
   Alpha0 type = fitting
   Beta0 type = fitting
   SigmaSquared type = fitting

.. categories::
