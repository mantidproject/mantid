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
the same manner as done by RemameWorkspace.

The new names can be either explicitly defined by a comma separated list
or by adding a prefix, suffix or both a prefix and suffix.

**Warning:** No new name can be the same as any existing workspace, even
if that existing workspace is also being renamed. Duplicate names may
cause the loss of a workspace.

.. categories::
