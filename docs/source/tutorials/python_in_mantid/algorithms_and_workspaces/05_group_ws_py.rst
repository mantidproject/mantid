.. _05_group_ws_py:

===========================
WorkspaceGroup with Python
===========================


A :ref:`WorkspaceGroup` is a workspace type that acts as a container for other workspaces. A WorkspaceGroup may be formed from any type of workspace and constructed on an arbitrary basis. :ref:`WorkspaceGroups <WorkspaceGroup>` are also used to store multi-period data at ISIS.

"*A workspace group is essentially an array of workspaces*."

Determine if this is a groups workspace

.. code-block:: python

    Load(Filename="MUSR00015189", OutputWorkspace="groupWS")
    groupWS = mtd["groupWS"]
    print("Workspace Type: " + groupWS.id())

Looping over a WorkspaceGroup

.. code-block:: python

    for i in range(groupWS.size()):
        print(groupWS[i].name())

or, like this ...

.. code-block:: python

    for ws in groupWS:
        print(ws.name())

Determine if this is multi-period data

.. code-block:: python

    print("Is multi-period data: {}".format(str(groupWS.isMultiPeriod())))
