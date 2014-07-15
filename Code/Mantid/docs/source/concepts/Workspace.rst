.. _Workspace:

Workspace
=========

What are they?
--------------

Workspaces are the nouns of Mantid (while `algorithms <algorithm>`__ are
the verbs). Workspaces hold the data in Mantid.

They come in several forms, but the most common by far is the
`MatrixWorkspace <MatrixWorkspace>`__ which contains measured or derived
data with associated errors. Matrix Workspaces are typically created
initially by executing one of Mantid's 'Load' algorithms, for example
`LoadRaw <http://docs.mantidproject.org/nightly/algorithms/LoadRaw.html>`__
or
`LoadNexus <http://docs.mantidproject.org/nightly/algorithms/LoadNexus.html>`__,
or they are the output of algorithms which took a matrix workspace as
input. In `MantidPlot <MantidPlot:_Help>`__ the data from the workspace
can viewed as a table, and graphed in many ways.

Another form of workspace is the `TableWorkspace <Table Workspaces>`__.
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
themselves are loaded in via Mantid's `plugin <plugin>`__ mechanism and
are created using the `Workspace Factory <Workspace Factory>`__.

Example Workspaces
------------------

-  `MatrixWorkspace <MatrixWorkspace>`__ - A base class that contains
   among others:

   -  `Workspace2D <Workspace2D>`__ - A workspace for holding two
      dimensional data in memory, this is the most commonly used
      workspace.
   -  `EventWorkspace <EventWorkspace>`__ - A workspace that retains the
      individual neutron event data.

-  `TableWorkspace <Table Workspaces>`__ - A workspace holding data in
   rows of columns having a particular type (e.g. text, integer, ...).
-  `WorkspaceGroup <WorkspaceGroup>`__ - A container for a collection of
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
| "EventWorkspace"              | `EventWorkspace <EventWorkspace>`__       |
+-------------------------------+-------------------------------------------+
| "ManagedWorkspace2D"          | ManagedWorkspace2D                        |
+-------------------------------+-------------------------------------------+
| "TableWorkspace"              | TableWorkspace                            |
+-------------------------------+-------------------------------------------+
| "Workspace2D"                 | `Workspace2D <Workspace2D>`__             |
+-------------------------------+-------------------------------------------+
| "WorkspaceSingleValue"        | WorkspaceSingleValue                      |
+-------------------------------+-------------------------------------------+
| "ManagedRawFileWorkspace2D"   | ManagedRawFileWorkspace2D                 |
+-------------------------------+-------------------------------------------+
| "MDEventWorkspace"            | `MDEventWorkspace <MDEventWorkspace>`__   |
+-------------------------------+-------------------------------------------+
| "MDHistoWorkspace"            | `MDHistoWorkspace <MDHistoWorkspace>`__   |
+-------------------------------+-------------------------------------------+



.. categories:: Concepts