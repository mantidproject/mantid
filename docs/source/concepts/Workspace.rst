.. _Workspace:

=========
Workspace
=========

.. contents::
  :local:

What are Workspaces?
--------------------

Workspaces store data that :ref:`algorithms <algorithm>` operate on. Workspaces are usually stored in-memory. An algorithm can manipulate a workspace in-place or create a new one as an output. 

Workspace is as loose term that encompases a range of possible data structures. All forms of Workspace provide some common information:

- The type identifier (id) of the workspace (see below), unique to every type of workspace 
- The name of the workspace, unique to every instance of a workspace even those of the same type
- The history of :ref:`algorithms <algorithm>` that were run to form and manipulate the workspace to its current state

.. tip:: In `MantidPlot <http://www.mantidproject.org/MantidPlot:_Help>`__ the data from the workspaces can be graphically viewed, inspected, and plotted in many ways.

.. note:: In addition to data, workspaces hold a :ref:`workspace  history <Workspace-Workspace_History>`, which details the algorithms which have been run on this workspace. This means workspaces carry all the meta-information to fully recreate themeselves.

Workspaces Types
------------------

-  :ref:`Matrix Workspace <MatrixWorkspace>` - Is really a catagorisation for a family which contains measured (or derived) data with associated errors and an axis giving information about where the the measurement was made. Matrix Workspaces are typically create initially by executing one of Mantid's :ref:`Load<algm-Load>` algorithms, for example :ref:`LoadRaw <algm-LoadRaw>` or :ref:`LoadNexus <algm-LoadNexus>`. Of data structures representing 2D measurement data. The following are common sorts of Matrix Workspace:

   -  :ref:`Workspace 2D <Workspace2D>` - A workspace for holding two
      dimensional histogram data in memory, this is the most commonly used
      workspace.
   -  :ref:`Event Workspace <EventWorkspace>` - A workspace that retains the
      individual neutron event data often including the pulse time correponding to the reading.

-  :ref:`Table Workspace <Table Workspaces>` - A workspace holding data in
   rows of columns having a particular type (e.g. text, integer, ...).
-  :ref:`Workspace Group <WorkspaceGroup>` - A container for a collection of
   workspaces. Algorithms given a group as input usually run sequentially on
   each member of the group.

The workspace type id identifies the type of a Workspace instance.  

.. tip:: For C++ or Python development, these values are needed in such functions as the :ref:`Analysis Data Service's <Analysis Data Service>` createWorkspace.

+-------------------------------+-------------------------------------------+
| ID                            | Workspace Type                            |
+===============================+===========================================+
| "IEventWorkspace"             | IEventWorkspace                           |
+-------------------------------+-------------------------------------------+
| "ITableWorkspace"             | ITableWorkspace                           |
+-------------------------------+-------------------------------------------+
| "WorkspaceGroup"              | WorkspaceGroup                            |
+-------------------------------+-------------------------------------------+
| "AbsManagedWorkspace2D"       | AbsManagedWorkspace2D                     |
+-------------------------------+-------------------------------------------+
| "CompressedWorkspace2D"       | CompressedWorkspace2D                     |
+-------------------------------+-------------------------------------------+
| "EventWorkspace"              | :ref:`EventWorkspace <EventWorkspace>`    |
+-------------------------------+-------------------------------------------+
| "ManagedWorkspace2D"          | ManagedWorkspace2D                        |
+-------------------------------+-------------------------------------------+
| "TableWorkspace"              | TableWorkspace                            |
+-------------------------------+-------------------------------------------+
| "Workspace2D"                 | :ref:`Workspace2D <Workspace2D>`          |
+-------------------------------+-------------------------------------------+
| "WorkspaceSingleValue"        | WorkspaceSingleValue                      |
+-------------------------------+-------------------------------------------+
| "ManagedRawFileWorkspace2D"   | ManagedRawFileWorkspace2D                 |
+-------------------------------+-------------------------------------------+
| "MDWorkspace"                 | :ref:`MDWorkspace <MDWorkspace>`          |
+-------------------------------+-------------------------------------------+
| "MDHistoWorkspace"            | :ref:`MDHistoWorkspace <MDHistoWorkspace>`|
+-------------------------------+-------------------------------------------+

Working with Workspaces
-----------------------

This :ref:`page <WorkingWithWorkspaces>` describes how you can work with workspaces in python, including accessing their properties and history

Writing you own Workspace
-------------------------

:ref:`Table Workspace <Table Workspaces>` is the best solution at present for customising the data structures you need. Changes beyond that are at present not trivial. For specialisation of exisiting data structures, or new data requirements, please contact the Mantid Team for help.


.. categories:: Concepts
