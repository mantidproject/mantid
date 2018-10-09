.. _WorkingWithWorkspaces:

Working with Workspaces in Python
---------------------------------

.. _Workspace-Accessing_Workspaces:

Accessing Workspaces
####################

In ``mantid.simpleapi``, you can access existing workspaces as a dictionary using the ``mtd["worskpace_name"]`` command for a specific workspace name key.  More explanation can be found in `Accessing Workspaces From Python <http://www.mantidproject.org/Accessing_Workspaces_From_Python>`_.

.. testcode:: AccessingWorkspaces

    # This creates a workspace without explicitly capturing the output
    CreateSampleWorkspace(OutputWorkspace="my_new_workspace")

    # You can get a python variable pointing to the workspace with the command
    myWS = mtd["my_new_workspace"]
    print("The variable myWS now points to the workspace called " + str(myWS))

    # You can assign a python variable when calling an algorithm and the workspace will match the variable name
    myOtherWS = CreateSampleWorkspace()
    print("myOtherWS now points to the workspace called " + str(myOtherWS))

Output:

.. testoutput:: AccessingWorkspaces
    :options: +NORMALIZE_WHITESPACE

    The variable myWS now points to the workspace called my_new_workspace
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

.. categories:: Concepts
