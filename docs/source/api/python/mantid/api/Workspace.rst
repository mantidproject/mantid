.. _mantid.api.Workspace:

===========
 Workspace
===========

This is a Python binding to the C++ class Mantid::API::Workspace.

*bases:* :py:obj:`mantid.kernel.DataItem`

What are Workspaces?
--------------------

Workspaces store data that :py:obj:`algorithms <mantid.api.Algorithm>` operate on. Workspaces are usually stored in-memory. An algorithm can manipulate a workspace in-place or create a new one as an output.

Workspace is as loose term that encompases a range of possible data structures. All forms of Workspace provide some common information:

- The type identifier (id) of the workspace (see below), unique to every type of workspace
- The name of the workspace, unique to every instance of a workspace even those of the same type
- The history of :py:obj:`algorithms <mantid.api.Algorithm>` that were run to form and manipulate the workspace to its current state

.. tip:: In :ref:`MantidWorkbench <workbench>` the data from the workspaces can be graphically viewed, inspected, and plotted in many ways.

.. note:: In addition to data, workspaces hold a :ref:`workspace  history <Workspace-Workspace_History>`, which details the algorithms which have been run on this workspace. This means workspaces carry all the meta-information to fully recreate themeselves.

Workspaces Types
------------------

-  :py:obj:`MatrixWorkspace <mantid.api.MatrixWorkspace>` - Is really a categorisation for a family which contains measured (or derived) data with associated errors and an axis giving information about where the measurement was made. Matrix Workspaces are typically create initially by executing one of Mantid's :ref:`Load<algm-Load>` algorithms, for example :ref:`LoadRaw <algm-LoadRaw>` or :ref:`LoadNexus <algm-LoadNexus>`. Of data structures representing 2D measurement data. The following are common sorts of Matrix Workspace:

   -  :py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>` - A workspace for holding two
      dimensional histogram data in memory, this is the most commonly used
      workspace.
   -  :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>` - A workspace that retains the
      individual neutron event data often including the pulse time corresponding to the reading.

-  :py:obj:`TableWorkspace <mantid.api.ITableWorkspace>` - A workspace holding data in
   rows of columns having a particular type (e.g. text, integer, ...).
-  :py:obj:`WorkspaceGroup <mantid.api.WorkspaceGroup>` - A container for a collection of
   workspaces. Algorithms given a group as input usually run sequentially on
   each member of the group.

The workspace type id identifies the type of a Workspace instance.

.. tip:: For C++ or Python development, these values are needed in such functions as the :py:obj:`AnalysisDataService <mantid.api.AnalysisDataServiceImpl>` createWorkspace.

+-------------------------------+-----------------------------------------------------------------+
| ID                            | Workspace Type                                                  |
+===============================+=================================================================+
| "IEventWorkspace"             | IEventWorkspace                                                 |
+-------------------------------+-----------------------------------------------------------------+
| "ITableWorkspace"             | ITableWorkspace                                                 |
+-------------------------------+-----------------------------------------------------------------+
| "WorkspaceGroup"              | WorkspaceGroup                                                  |
+-------------------------------+-----------------------------------------------------------------+
| "AbsManagedWorkspace2D"       | AbsManagedWorkspace2D                                           |
+-------------------------------+-----------------------------------------------------------------+
| "CompressedWorkspace2D"       | CompressedWorkspace2D                                           |
+-------------------------------+-----------------------------------------------------------------+
| "EventWorkspace"              | :py:obj:`EventWorkspace <mantid.dataobjects.EventWorkspace>`    |
+-------------------------------+-----------------------------------------------------------------+
| "ManagedWorkspace2D"          | ManagedWorkspace2D                                              |
+-------------------------------+-----------------------------------------------------------------+
| "TableWorkspace"              | TableWorkspace                                                  |
+-------------------------------+-----------------------------------------------------------------+
| "Workspace2D"                 | :py:obj:`Workspace2D <mantid.dataobjects.Workspace2D>`          |
+-------------------------------+-----------------------------------------------------------------+
| "WorkspaceSingleValue"        | WorkspaceSingleValue                                            |
+-------------------------------+-----------------------------------------------------------------+
| "ManagedRawFileWorkspace2D"   | ManagedRawFileWorkspace2D                                       |
+-------------------------------+-----------------------------------------------------------------+
| "MDWorkspace"                 | :py:obj:`MDWorkspace <mantid.api.IMDWorkspace>`                 |
+-------------------------------+-----------------------------------------------------------------+
| "MDHistoWorkspace"            | :py:obj:`MDHistoWorkspace <mantid.dataobjects.MDHistoWorkspace>`|
+-------------------------------+-----------------------------------------------------------------+

Working with Workspaces
-----------------------

This :ref:`page <WorkingWithWorkspaces>` describes how you can work with workspaces in python, including accessing their properties and history

Writing your own Workspace
--------------------------

:py:obj:`TableWorkspace <mantid.api.ITableWorkspace>` is the best solution at present for customising the data structures you need.Changes beyond that are at present not trivial.
For specialisation of existing data structures, or new data requirements, please contact the Mantid Team for help.


Reference
---------

.. module:`mantid.api`

.. autoclass:: mantid.api.Workspace
    :members:
    :undoc-members:
    :inherited-members:
