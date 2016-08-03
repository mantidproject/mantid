.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves a TableWorkspace containing Diffraction Fitting results in to an ASCII file in TBL format.
All of the columns must be typed as 'double' in order to successfully save it as a ASCII file.
A GroupWorkspace of TableWorkspace can also be saved where the run number and bank is provided
manually as a list in the correct order.



Limitations
###########

The Algorithm will fail if any of the columns is not of type 'double'.

Usage
-----

**Example - Save a TableWorkspace in Diffraction Fitting Table format**

.. testcode:: ExSaveDiffFittingAscii

    #import the os path libraries for directory functions
    import os

    # Create a table workspace with data to save.
    ws = CreateEmptyTableWorkspace()
    ws.addColumn("double", "dSpacing");
    ws.addColumn("double", "A0");
    ws.addColumn("double", "A0_Err");
    ws.addColumn("double", "A1");
    ws.addColumn("double", "A1_Err");
    ws.addColumn("double", "X0");
    ws.addColumn("double", "X0_Err");
    ws.addColumn("double", "A");
    ws.addColumn("double", "A_Err");
    ws.addColumn("double", "B");
    ws.addColumn("double", "B_Err");
    ws.addColumn("double", "S");
    ws.addColumn("double", "S_Err");
    ws.addColumn("double", "I");
    ws.addColumn("double", "I_Err");
    ws.addColumn("double", "Chi");

    nextRow = {'dSpacing': 1.6092, 'A0': 0.015160, 'A0_Err': 5.41233, 'A1': -1.93673e-06, 'A1_Err': 0.00018,
           'X0': 0.00018, 'X0_Err': 5.8782518, 'A': 0.021762, 'A_Err': 0.003330, 'B': 0.023050,
           'B_Err': 0.0035287, 'S': 21.99236, 'S_Err': 7.1390246, 'I': 521.0432, 'I_Err': 27.212413,
           'Chi': 3.0258827}
    ws.addRow(nextRow)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "FittingResults.txt")

    # perform the algorithm
    SaveDiffFittingAscii(InputWorkspace = ws, Filename=savefile, RunNumber="21344", Bank = "1",
    OutFormat = "AppendToExistingFile")


    print "File Exists:", os.path.exists(savefile)

.. testcleanup:: ExSaveDiffFittingAscii

    os.remove(savefile)

Output:

.. testoutput:: ExSaveDiffFittingAscii

    File Exists: True

.. categories::

.. sourcelink::
