:orphan:

.. testcode:: mwTest_Python_WorkspaceGroup_v2[5]

   Load(Filename="MUSR00015189", OutputWorkspace="groupWS")
   groupWS = mtd["groupWS"]
   print "Workspace Type: ", groupWS.id()

.. testoutput:: mwTest_Python_WorkspaceGroup_v2[5]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Workspace Type: WorkspaceGroup


.. testsetup:: mwTest_Python_WorkspaceGroup_v2[15]

   Load(Filename="MUSR00015189", OutputWorkspace="groupWS")
   groupWS = mtd["groupWS"]

.. testcode:: mwTest_Python_WorkspaceGroup_v2[15]

   for i in range(groupWS.size()):
   	print groupWS[i].name()

.. testoutput:: mwTest_Python_WorkspaceGroup_v2[15]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   groupWS_1
   groupWS_2


.. testsetup:: mwTest_Python_WorkspaceGroup_v2[29]

   Load(Filename="MUSR00015189", OutputWorkspace="groupWS")
   groupWS = mtd["groupWS"]

.. testcode:: mwTest_Python_WorkspaceGroup_v2[29]

   for ws in groupWS:
   	print ws.name()

.. testoutput:: mwTest_Python_WorkspaceGroup_v2[29]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   groupWS_1
   groupWS_2


.. testsetup:: mwTest_Python_WorkspaceGroup_v2[43]

   Load(Filename="MUSR00015189", OutputWorkspace="groupWS")
   groupWS = mtd["groupWS"]

.. testcode:: mwTest_Python_WorkspaceGroup_v2[43]

   print "Is multi-period data: ", groupWS.isMultiPeriod()

.. testoutput:: mwTest_Python_WorkspaceGroup_v2[43]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Is multi-period data:  False


