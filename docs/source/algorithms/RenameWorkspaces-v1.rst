.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Renames a list of workspaces in the data service. This renaming is done
by either replacing with new names in a list or adding a prefix, suffix
or both prefix and suffix. The Renaming is implemented by calling
RenameWorkspace as a child algorithm having defined the output workspace
appropriately.

If run on a group workspace, the members of the group will be renamed in
the same manner as done by RenameWorkspace.

The new names can be either explicitly defined by a comma separated list
or by adding a prefix, suffix or both a prefix and suffix.

The new name can be the same as any existing workspaces if the overwrite flag
is set to true. This will replace any existing workspaces with the same output 
name.
If the overwrite flag is set to false and any name is in use RenameWorkspaces
will not rename any of the workspaces and log an error.

.. warning::
   RenameWorkspaces is set to overwrite by default to maintain backwards compatibility. 
   Ensure the overwrite flag is set to false to prevent any existing workspaces being 
   replaced with a renamed workspace.

Usage
-----

**Example - New names:**

.. testcode:: ExNewNames

   # Make sure there is nothing in the ADS before we begin
   mtd.clear()

   names = ['ws1', 'ws2', 'ws3']

   # Create some dummy workspaces
   for name in names:
     CreateWorkspace([0], [0], OutputWorkspace=name)

   ws_before_rename = mtd.getObjectNames()
   ws_before_rename.sort()
   print('Workspaces in the ADS _before_ renaming: {}'.format(ws_before_rename))

   RenameWorkspaces(names, WorkspaceNames=['new_ws1', 'new_ws2', 'new_ws3'])

   ws_after_rename = mtd.getObjectNames()
   ws_after_rename.sort()
   print('Workspaces in the ADS _after_ renaming: {}'.format(ws_after_rename))

Output:

.. testoutput:: ExNewNames

   Workspaces in the ADS _before_ renaming: ['ws1', 'ws2', 'ws3']
   Workspaces in the ADS _after_ renaming: ['new_ws1', 'new_ws2', 'new_ws3']

**Example - Using prefix and suffix:**

.. testcode:: ExPrefixAndSuffix

   # Make sure there is nothing in the ADS before we begin
   mtd.clear()

   names = ['ws1', 'ws2', 'ws3']

   # Create some dummy workspaces
   for name in names:
     CreateWorkspace([0], [0], OutputWorkspace=name)

   ws_before_rename = mtd.getObjectNames()
   ws_before_rename.sort()
   print('Workspaces in the ADS _before_ renaming: {}'.format(ws_before_rename))

   RenameWorkspaces(names, Prefix='new_', Suffix='_name')

   ws_after_rename = mtd.getObjectNames()
   ws_after_rename.sort()
   print('Workspaces in the ADS _after_ renaming: {}'.format(ws_after_rename))

Output:

.. testoutput:: ExPrefixAndSuffix

   Workspaces in the ADS _before_ renaming: ['ws1', 'ws2', 'ws3']
   Workspaces in the ADS _after_ renaming: ['new_ws1_name', 'new_ws2_name', 'new_ws3_name']

**Example - Setting overwrite on and off:**

.. testcode:: ExOverwriteExisting

   #Clear the ADS before starting
   mtd.clear()
       
   #Create an existing workspace called 'new_ws1'
   CreateWorkspace([0], [0], OutputWorkspace="new_ws1")
       
   #Next create workspaces we are going to rename
   names = ['ws1', 'ws2', 'ws3']
   
   for name in names:
       CreateWorkspace([0], [0], OutputWorkspace=name)
       
   #This will fail telling us that 'new_ws1' already exists
   print('Trying to rename with OverwriteExisting set to false.')
   try:
       RenameWorkspaces(names, Prefix='new_', OverwriteExisting=False)
   except RuntimeError:
       print('RuntimeError: A workspace called new_ws1 already exists')
   
   #This will succeed in renaming and 'new_ws1' will be replaced with 'ws1'
   print('Trying to rename with OverwriteExisting set to true.')
   RenameWorkspaces(names, Prefix='new_', OverwriteExisting=True)   
   print('Succeeded')
   
Output:

.. testoutput:: ExOverwriteExisting
   
   Trying to rename with OverwriteExisting set to false.
   RuntimeError: A workspace called new_ws1 already exists
   Trying to rename with OverwriteExisting set to true.
   Succeeded
   
.. categories::

.. sourcelink::
