.. _Workspace:

=========
Workspace
=========

.. contents::
  :local:

What are Workspaces?
--------------------

Workspaces are the nouns of Mantid (while :ref:`algorithms <algorithm>` are
the verbs). Workspaces hold the data in Mantid.

They come in several forms, but the most common by far is the
:ref:`MatrixWorkspace <MatrixWorkspace>` which contains measured or derived
data with associated errors. Matrix Workspaces are typically created
initially by executing one of Mantid's :ref:`Load<algm-Load>` algorithms, for example
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

In addition to data, workspaces hold a :ref:`workspace  history <Workspace-Workspace_History>`,
which details the algorithms which have been run on this workspace.

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

Working with Workspaces in Python
---------------------------------

Workspace is an abstract description of an specific workspace implementation. It provides access to a few common properties without any knowledge of what the type of the workspace.

.. _Workspace-Accessing_Workspaces:

Accessing Workspaces
####################

You can access workspaces using the ``mtd["worskpace_name"]`` command for a specific workspace, or using the ``mtd.ImportAll()`` to create python variables for every workspace in Mantid.  More explanation can be found in `Accessing Workspaces From Python <http://www.mantidproject.org/Accessing_Workspaces_From_Python>`_.

.. testcode:: AccessingWorkspaces

    # This creates a workspace without explicitly capturing the output
    CreateSampleWorkspace(OutputWorkspace="MyNewWorkspace")

    # You can get a python variable pointing to the workspace with the command
    myWS = mtd["MyNewWorkspace"]
    print("The variable myWS now points to the workspace called " + str(myWS))

    # You can also ask Mantid to create matching python variables for all of it's workspaces
    mtd.importAll()
    print("MyNewWorkspace has been created that also points to the workspace called " + str(MyNewWorkspace))

    # You can assign a python variable when calling an algorithm and the workspace will match the variable name
    myOtherWS = CreateSampleWorkspace()
    print("myOtherWS now points to the workspace called " + str(myOtherWS))

Output:

.. testoutput:: AccessingWorkspaces
    :options: +NORMALIZE_WHITESPACE

    The variable myWS now points to the workspace called MyNewWorkspace
    MyNewWorkspace has been created that also points to the workspace called MyNewWorkspace
    myOtherWS now points to the workspace called myOtherWS

Workspace Properties
####################

You can look at the :ref:`Workspace API reference <mantid.api.Workspace>` for a full list of properties, but here are some of the key ones.

.. testcode:: WorkspaceProperties

    myWS = CreateSampleWorkspace()
    print("name = " + myWS.name())

    myWS.setTitle("This is my Title")
    print("getTitle = " + myWS.getTitle())

    myWS.setComment("This is my comment")
    print("comment = " + myWS.getComment())

    print("id = " + myWS.id())

    print("getMemorySize = " + str(myWS.getMemorySize()))

Output:

.. testoutput:: WorkspaceProperties
    :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

    name = myWS
    getTitle = This is my Title
    comment = This is my comment
    id = Workspace2D
    getMemorySize = ...


Workspace Types
^^^^^^^^^^^^^^^

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

.. _Workspace-Workspace_History:

Workspace History
#################

Workspaces keep a track of all of the algorithms used on them, so you can ask a workspace to tell you about it's history.  The algorithm :ref:`GeneratePythonScript <algm-GeneratePythonScript>` uses this information to create a python script able to re-run the workspace history.

.. testcode:: WorkspaceHistory

    # Run a few algorithms
    myWS = CreateSampleWorkspace()
    myWS = ConvertUnits(myWS,Target="Wavelength")
    myWS = Rebin(myWS,Params=200)

    # You can access the history using getHistory()
    history = myWS.getHistory()
    for algHistory in history.getAlgorithmHistories():
        print(algHistory.name())
        for property in algHistory.getProperties():
            if not property.isDefault():
                print("\t" + property.name() + " = " + property.value())

Output:

.. testoutput:: WorkspaceHistory
    :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

    CreateSampleWorkspace
        OutputWorkspace = myWS
    ConvertUnits
        InputWorkspace = myWS
        OutputWorkspace = myWS
        Target = Wavelength
    Rebin
        InputWorkspace = myWS
        OutputWorkspace = myWS
        Params = 200
        
The full documentation for workspace history can be found at the :class:`~mantid.api.WorkspaceHistory` api.

Writing you own workspace
-------------------------

This is perfectly possible, but not as easy as creating your own
algorithm. Please talk to a member of the development team if you wish
to implement you own workspace.

.. categories:: Concepts
