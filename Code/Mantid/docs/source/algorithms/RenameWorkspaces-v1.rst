.. algorithm::

.. summary::

.. alias::

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

.. warning::

   No new name can be the same as any existing workspace, even if that
   existing workspace is also being renamed. Duplicate names may cause
   the loss of a workspace.

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

   print 'Workspaces in the ADS _before_ renaming:', mtd.getObjectNames()

   RenameWorkspaces(names, WorkspaceNames=['new_ws1', 'new_ws2', 'new_ws3'])

   print 'Workspaces in the ADS _after_ renaming:', mtd.getObjectNames()

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

   print 'Workspaces in the ADS _before_ renaming:', mtd.getObjectNames()

   RenameWorkspaces(names, Prefix='new_', Suffix='_name')

   print 'Workspaces in the ADS _after_ renaming:', mtd.getObjectNames()

Output:

.. testoutput:: ExPrefixAndSuffix

   Workspaces in the ADS _before_ renaming: ['ws1', 'ws2', 'ws3']
   Workspaces in the ADS _after_ renaming: ['new_ws1_name', 'new_ws2_name', 'new_ws3_name']

.. categories::
