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

**Running the FlatCell Algorithm with a 2D Workspace**

.. testcode:: exFlatCell

    # Load data file from the csv file
    input = LoadAscii(Filename="flatcell_input.csv", OutputWorkspace="FlatCellInput")

    # Run the FlatCell Algorithm and save into output workspaces
    FlatCell(InputWorkspace="FlatCellInput", OutputWorkspace="FlatCellOuput")

    # Verify the changes are correct
    print("The range of the input values is {}.".format(np.ptp(mtd['FlatCellInput'].readY(0))))
    print("The range of the output values is {}.".format(np.ptp(mtd['FlatCellOuput'].readY(0))))


Output:

.. testoutput:: exFlatCell

   The range of the input values is 32075.0.
   The range of the output values is 527.7157520726411.

.. categories::

.. sourcelink::
