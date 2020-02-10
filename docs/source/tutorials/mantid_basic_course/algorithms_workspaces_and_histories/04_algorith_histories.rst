.. _04_algorith_histories:

====================
Algorithim Histories 
====================

Algorithm History
=================

Mantid keeps the entire history of all algorithms applied to workspaces.
Not only does this allow you to audit the data reduction and analysis,
it also provides the means to extract a re-executable python script from
the GUI.

#. Right click on the Workspace *Rebin300* and select 'Show History'. This will
   open up the Algorithm History window. In the list of Algorithms, 
   click on Rebin v.1 and you should see the following:

   |ThisFilenameislying|
   
   This reveals the history done to this workspace, i.e. Load to load
   the workspace, SumSpectra, and Rebin.
   To replay the history do:
#. Go back to Algorithm History window and press the 'Script to
   Clipboard' button and close the Algorithm History window
#. In the central box of the main Mantid window, the script Editor, paste
   what was copied to the clipboard.
#. Close the HYS_11388_event plot window
#. Delete the HYS_11388_event workspace from the Workspaces Toolbox
#. To recreate the work you have just done, Execute the script by clicking on the green run arrow.
#. To verify that history has been replayed either look at history or
   plot the spectrum again.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Workspaces|Mantid_Basic_Course|MBC_Interfaces}}

.. |ThisFilenameislying| image:: /images/HistoryRebinOfCNCS_7860_event.png
