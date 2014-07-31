.. _Workspace:

Workspace
=========

What are they?
--------------

Workspaces are the nouns of Mantid (while :ref:`algorithms <algorithm>` are
the verbs). Workspaces hold the data in Mantid.

They come in several forms, but the most common by far is the
:ref:`MatrixWorkspace <MatrixWorkspace>` which contains measured or derived
data with associated errors. Matrix Workspaces are typically created
initially by executing one of Mantid's 'Load' algorithms, for example
:ref:`LoadRaw <algm-LoadRaw>`
or
:ref:`LoadNexus <algm-LoadNexus>`,
or they are the output of algorithms which took a matrix workspace as
input. In `MantidPlot <http://www.mantidproject.org/MantidPlot:_Help>`__ the data from the workspace
can viewed as a table, and graphed in many ways.

Another form of workspace is the :ref:`TableWorkspace <Table Workspaces>`.
This stores data of (somewhat) arbitrary type in rows and columns, much
like a spreadsheet. These typically are created as the output of certain
specialized algorithms (e.g. curve fitting).

In addition to data, workspaces hold a `workspace
history <WorkspaceHistory>`__, which details the algorithms which have
been run on this workspace.

In software engineering terms, the 'abstract' concept of a workspace is
an 'interface', in that it defines common properties that are
implemented by various 'concrete' workspaces. Interaction with
workspaces is typically through an interface. The concrete workspaces
themselves are loaded in via Mantid's :ref:`plugin <plugin>` mechanism and
are created using the Workspace Factory.

Example Workspaces
------------------

-  :ref:`MatrixWorkspace <MatrixWorkspace>` - A base class that contains
   among others:

   -  :ref:`Workspace2D <Workspace2D>` - A workspace for holding two
      dimensional data in memory, this is the most commonly used
      workspace.
   -  :ref:`EventWorkspace <EventWorkspace>` - A workspace that retains the
      individual neutron event data.

-  :ref:`TableWorkspace <Table Workspaces>` - A workspace holding data in
   rows of columns having a particular type (e.g. text, integer, ...).
-  :ref:`WorkspaceGroup <WorkspaceGroup>` - A container for a collection of
   workspaces. Algorithms given a group as input run sequentially on
   each member of the group.

Writing you own workspace
-------------------------

This is perfectly possible, but not as easy as creating your own
algorithm. Please talk to a member of the development team if you wish
to implement you own workspace.

Workspace Types
---------------

The workspace type id identifies the type (underlying class) of a
Workspace object. These IDs are listed here for ease of reference, so
you needn't navigate Doxygen for a list of workspace types. These values
are needed in such functions as the AnalysisDataService's
createWorkspace if you are writing C++ or Python algorithms.

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



.. categories:: Concepts