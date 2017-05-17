.. _WorkspaceGroup:

===============
Workspace Group
===============


.. contents::
  :local:

A WorkspaceGroup is a group of workspaces.

Most algorithms will execute on a WorkspaceGroup by simply executing the
algorithm on each workspace contained within.

Creating a Workspace Group
~~~~~~~~~~~~~~~~~~~~~~~~~~

-  Select a few workspaces in MantidPlot and click the "Group" button
   above the list of workspaces.
-  Use the :ref:`GroupWorkspaces <algm-GroupWorkspaces>` algorithm.

Un-grouping Workspaces
~~~~~~~~~~~~~~~~~~~~~~

-  Select the WorkspaceGroup and click "Ungroup".
-  Use the :ref:`UnGroupWorkspace <algm-UnGroupWorkspace>` algorithm.

Working with Event Workspaces in Python
----------------------------------------

Creating and splitting groups
#############################

.. testcode:: CreatingWorkspaceGroups

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()


    # Create a group workpace
    wsList = [ws1,ws2,ws3]
    wsGroup = GroupWorkspaces(wsList)
    # or 
    wsGroup = GroupWorkspaces("ws1,ws2,ws3")

    print wsGroup.getNames()

    # Remove the group
    # The child workspaces will be preserved
    UnGroupWorkspace(wsGroup)
    # Using wsGroup now will cause a runtime error
    # RuntimeError: Variable invalidated, data has been deleted.


.. testoutput:: CreatingWorkspaceGroups
    :hide:
    :options: +NORMALIZE_WHITESPACE

    ['ws1','ws2','ws3']

Accessing Workspace Groups
##########################

The methods for getting a variable to an EventWorkspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is a Workspace Group you can use this:

.. testcode:: CheckGroupWorkspace

    from mantid.api import WorkspaceGroup

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces("ws1,ws2")

    if isinstance(wsGroup, WorkspaceGroup):
        print wsGroup.name() + " is an " + wsGroup.id()

Output:

.. testoutput:: CheckGroupWorkspace
    :options: +NORMALIZE_WHITESPACE

    wsGroup is an WorkspaceGroup

Looping over all of the members of a group
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. testcode:: GroupWorkspaceMembers

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces("ws1,ws2")

    print "Number of members:", wsGroup.getNumberOfEntries()
    print "List of names:", wsGroup.getNames()

    # Get the member workspaces in a loop
    for i in range(wsGroup.getNumberOfEntries()):
        wsLoop = wsGroup.getItem(i)
        print "Member", i, wsLoop


Output:

.. testoutput:: GroupWorkspaceMembers
    :options: +NORMALIZE_WHITESPACE

    Number of members: 2
    List of names: ['ws1','ws2']
    Member 0 ws1
    Member 1 ws2

Using Workspace Groups in Algorithms
####################################

You can pass workspace groups into any algorithm and Mantid will run that algorithm for each member of the workspace group.

.. testcode:: CheckGroupWorkspace

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    wsGroup = GroupWorkspaces("ws1,ws2")
    wsGroup2 = GroupWorkspaces("ws2,ws1")

    #  This will add the member workspaces in a pair like manner
    wsGroup3 = wsGroup + wsGroup2

    # Rebin all of wsGroup
    wsRebinned = Rebin(wsGroup, Params=200)

    # You can still of course refer to members of a group directly
    ws1 = Rebin(ws1, Params=100)

.. include:: WorkspaceNavigation.txt



.. categories:: Concepts
