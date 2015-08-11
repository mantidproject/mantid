:orphan:

.. testcode:: mwTest_Workspace_Types_Via_Python[15]

   WS = Load(Filename="GEM38370_Focussed.nxs", OutputWorkspace="myWS")
   print WS.getComment()
   print WS.getMemorySize()
   print WS.getName()
   print WS.getTitle()
   print WS

.. testoutput:: mwTest_Workspace_Types_Via_Python[15]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ...
   myWS
   38370 Martin,Damay,Mannini MnV2O4 28K _T in cryomag 20-APR-2008 15:33:20
   myWS


