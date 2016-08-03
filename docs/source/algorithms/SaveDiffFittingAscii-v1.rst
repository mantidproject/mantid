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


The following TableWorkspace:

becomes:

::

  run number: 23424
  bank: 1
  dSpacing,A0,A0_Err,A1,A1_Err,X0,X0_Err,A,A_Err,B,B_Err,S,S_Err,I,I_Err,Chi
  1.6092,0.01516,5.4123299999999999,-1.93673e-06,0.00018000000000000001,0.00018000000000000001,5.8782518000000001,0.021762,0.0033300000000000001,0.023050000000000001,0.0035287000000000001,21.992360000000001,7.1390245999999999,521.04319999999996,27.212413000000002,3.0258826999999999
  1.6834,0.17754,5.4123299999999999,3.9826001040999999,0.00012,0.00018000000000000001,30963.562000000002,0.021762,4.0946689999999997,0.10478999999999999,44.493605000000002,0.060733000000000002,24.092390999999999,801.06389999999999,23.939609999999998,3.0258826999999999
  -0.054739999999999997,7.0641651999999997,1.04e-05,-1.93673e-06,0.000194,36280.220000000001,7.9724626000000001,0.074860999999999997,0.039853899999999998,0.022008,0.00092670000000000003,38.47428,5.1456556999999998,6386.3040000000001,63.731360000000002,3.2363716

*This is just an example table, the values may not be valid.*

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

    nextRow = {'dSpacing': 1.6834, 'A0': 0.177540, 'A0_Err': 5.41233, 'A1': 3.9826001041, 'A1_Err': 0.00012,
           'X0': 0.00018, 'X0_Err': 30963.562, 'A': 0.021762, 'A_Err': 4.094669, 'B': 0.104790,
           'B_Err': 44.493605, 'S': 0.060733, 'S_Err': 24.092391, 'I': 801.0639, 'I_Err': 23.93961,
           'Chi': 3.0258827}
    ws.addRow(nextRow)

    nextRow = {'dSpacing': -0.05474, 'A0': 7.0641652, 'A0_Err': 1.04e-05, 'A1': -1.93673e-06, 'A1_Err': 0.000194,
           'X0': 36280.22, 'X0_Err': 7.9724626, 'A': 0.074861, 'A_Err': 0.0398539, 'B': 0.022008,
           'B_Err': 0.0009267, 'S': 38.47428, 'S_Err': 5.1456557, 'I': 6386.304, 'I_Err': 63.73136,
           'Chi': 3.2363716}
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
