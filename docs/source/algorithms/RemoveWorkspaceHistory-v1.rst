.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Removes all algorithm history records from a given workspace. This includes all workflow and child algorithm history records. After this algorithm has been run, the workspace will not be reproducible
from its history. Note that this algorithm will not remove the environment history associated with a workspace.

Usage
-----

**Example:**

.. testcode:: ExStripHistory

   def print_workspace_history(ws):
      history = ws.getHistory()
      print("Workspace {} has {} algorithms in it's history".format(ws,history.size()))
      for alg in history.getAlgorithmHistories():
         print("  " + alg.name())

   # create histogram workspace
   ws = CreateSampleWorkspace(NumBanks=1,BankPixelWidth=1)

   #create some history by running a few algs
   ws = ConvertUnits(ws,"Wavelength")
   ws = Rebin(ws,Params=1)
   ws += ws
   ws *= ws

   print_workspace_history(ws)

   RemoveWorkspaceHistory(ws)

   print("After RemoveWorkspaceHistory")
   print_workspace_history(ws)


Output:

.. testoutput:: ExStripHistory

   Workspace ws has 5 algorithms in it's history
     CreateSampleWorkspace
     ConvertUnits
     Rebin
     Plus
     Multiply
   After RemoveWorkspaceHistory
   Workspace ws has 1 algorithms in it's history
     RemoveWorkspaceHistory


.. categories::

.. sourcelink::
