.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Creates the FlatCell workspace for the ISIS SANS reduction for LOQ.
Outputs two Workspaces - The FlatCell and the MaskedBins Workspaces.


Usage
-----

**Running the GenerateFlatCellWorkspaceLOQ Algorithm with an event Workspace**

.. testcode:: exGenerateFlatCellWorkspaceLOQ

    # Load event workspace
    LoadNexus(Filename="LOQ00113953.nxs", OutputWorkspace="FlatCellInput")

    # Run the GenerateFlatCellWorkspaceLOQ Algorithm and save into output workspaces
    GenerateFlatCellWorkspaceLOQ(InputWorkspace="FlatCellInput", OutputWorkspace="FlatCellOuput")

    # Verify the changes are correct
    print("The range of the output values is {:.3f}.".format(np.ptp(mtd['FlatCellOuput'].extractY())))


Output:

.. testoutput:: exGenerateFlatCellWorkspaceLOQ

   The range of the output values is 1390.904.

.. categories::

.. sourcelink::
