.. _WorkspaceGroup:

===============
Workspace Group
===============


.. contents::
  :local:

A WorkspaceGroup is a group of workspaces. The WorkspaceGroup object does not hold any data itself, but instead holds a list of Workspace objects. They appear as an expandable list of workspaces in the MantidPlot interface (the list of workspaces is also called the ADS or *AnalysisDataService*). Thus, workspace groups add structure to the ADS and make it more readable and also allow algorithms to be executed over a list of workspaces contained within the group but passing the group to the algorithm.

Most algorithms can be passed a WorkspaceGroup in place of a normal workspace input, and will simply execute the algorithm on each workspace contained within the group.

Working with Event Workspaces in Python
----------------------------------------

Creating and splitting groups
#############################

Workspace groups can be created through the MantidPlot interface;

- Select a few workspaces from the ADS in MantidPlot and click the "Group" button above the list of workspaces. The group will be named "NewGroup".

Workspace groups can be created in a more flexible way in the Python script window using the Python API. Groups may be created via the :ref:`GroupWorkspaces <algm-GroupWorkspaces>` algorithm,  This will place a workspace group directly into the ADS, and requires at least one workspace to be added to the group.

.. testcode:: CreatingWorkspaceGroups

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()


    # Create a group workpace
    wsList = [ws1,ws2,ws3]
    wsGroup = GroupWorkspaces(wsList)
    # or 
    wsGroup = GroupWorkspaces("ws1,ws2,ws3")

    print(wsGroup.getNames())

    # Remove the group
    # The child workspaces will be preserved
    UnGroupWorkspace(wsGroup)
    # Using wsGroup now will cause a runtime error
    # RuntimeError: Variable invalidated, data has been deleted.

Output:

.. testoutput:: CreatingWorkspaceGroups
    :options: +NORMALIZE_WHITESPACE

    ['ws1','ws2','ws3']

To avoid interaction with the ADS, a `WorkspaceGroup` object can be instantiated using

.. code::

    import mantid.api as api

    ws_group = api.WorkspaceGroup()

This **will not** be automatically added to the ADS, to do so, use the following line

.. code::

    AnalysisDataService.add("name", ws_group)

the group should then appear in the ADS with the given name. Using direct instantiation; groups can be added to the ADS and then workspaces added to the group via their name and the `add` method;

.. testcode:: CreatingWorkspaceGroupsInstantiated

    from mantid.api import WorkspaceGroup

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()

    # Create a group workspace and add to the ADS
    ws_group = WorkspaceGroup()
    mtd.add("group1", ws_group)

    ws_group.add("ws1")
    ws_group.add("ws2")
    ws_group.add("ws3")

    print(ws_group.getNames())

Output:

.. testoutput:: CreatingWorkspaceGroupsInstantiated
    :options: +NORMALIZE_WHITESPACE

    ['ws1','ws2','ws3']

Alternatively, workspace group objects can be fed workspaces which are not in the ADS (in this case the `addWorkspace` method is used rather than `add` because `add` requires a name, and since the workspaces are not in the ADS they may not have a name)

.. testcode:: CreatingWorkspaceGroupsNoADS

    from mantid.api import WorkspaceGroup

    ws1 = WorkspaceFactory.create("Workspace2D", 2, 2, 2)
    ws2 = WorkspaceFactory.create("Workspace2D", 2, 2, 2)
    ws3 = WorkspaceFactory.create("Workspace2D", 2, 2, 2)

    # Create a group workspace
    ws_group = WorkspaceGroup()

    ws_group.addWorkspace(ws1)
    ws_group.addWorkspace(ws2)
    ws_group.addWorkspace(ws3)

    print(ws_group.getNames())

    mtd.add("group1", ws_group)

    print(ws_group.getNames())

Output:

.. testoutput:: CreatingWorkspaceGroupsNoADS
    :options: +NORMALIZE_WHITESPACE

    ['','','']
    ['group1_1','group1_2','group1_3']

when adding the group to the ADS, the workspaces will also be added, and given default names. It is not recommended to add workspace to groups in this way, as much of the functionality of groups depends on workspaces having names; for example the "in" keyword.

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
        print(wsGroup.name() + " is an " + wsGroup.id())

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

    print("Number of members: " + str(wsGroup.getNumberOfEntries()))
    print("List of names: " + str(wsGroup.getNames()))

    # Get the member workspaces in a loop
    for i in range(wsGroup.getNumberOfEntries()):
        wsLoop = wsGroup.getItem(i)
        print("Member {0} {1}".format(i, wsLoop.getName()))


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

Using Nested Workspace Groups
#############################

It is possible to have groups within groups, the inner group can simply be added to the outer group in the usual way. For example

.. code::

    # create the following structure
    # group1/
    #       ws1
    #       ws2
    #       group2/
    #               ws3
    #               ws4

    ws1 = CreateSampleWorkspace()
    ws2 = CreateSampleWorkspace()
    ws3 = CreateSampleWorkspace()
    ws4 = CreateSampleWorkspace()

    group1 = WorkspaceGroup()
    group2 = WorkspaceGroup()

    mtd.add("group1", group1)
    mtd.add("group2", group2)

    group1.add("ws1")
    group1.add("ws2")
    group2.add("ws3")
    group2.add("ws4")

    group1.add("group2")

Be careful when creating nested groups; every single group and workspace must have a unique name, if not workspaces with the same name will be overwritten. Do not try to group workspaces which are already in a group using `groupWorkspace` as this will create duplicate named workspaces in two groups; this will result in data being deleted without warning. Similarly don't try to create duplicate named workspaces and put them into different folders.

One final note; it is best to add all workspaces to the ADS before configuring the grouping structure (as in the above code); otherwise you will only be able to name the top level group when you add the structure to the ADS. All the sub-groups and workspaces not already in the ADS will be given default names which you will then have to change manually, it is much easier to name them as you go (and putting them in the ADS is the only way to name them).

.. include:: WorkspaceNavigation.txt



.. categories:: Concepts
