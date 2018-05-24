.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

InputWorskpace
##############

It can be either a MaskWorkspace, containing the masking information, or
a Data workspace (EventWorkspace or Workspace2D), having detectors
masked.

Optional MaskTableWorkspace
###########################

If the optional input 'MaskTableWorkspace' is given, it must be a table
workspace having the same format as output TableWorkspace such that it
contains 3 columns, XMin, XMax and DetectorIDsList.

The contents in this mask table workspace will be copied to output
workspace.

If a detector is masked in this input 'MaskTableWorkspace', and it is
also masked in input MaskWorkspace or data workspace, the setup (Xmin
and Xmax) in MaskTableWorkspace has higher priority, i.e., in the output
mask table workspace, the masked detector will be recorded in the row
copied from input MaskTableWrokspace.

Usage
-----

**Example - Extract mask to table without optional input table workspace:**

.. testcode:: ExHistSimple

  # load data
  dataws = LoadNexusProcessed(Filename="PG3_2538_2k.nxs")

  # mask some detectors
  for i in range(100):
      dataws.maskDetectors(100+i)

  # Run algorithm
  outmaskws = ExtractMaskToTable(InputWorkspace=dataws, Xmin = 12300., Xmax = 24500.)

  # Output
  print("Number of rows:  {}".format(outmaskws.rowCount()))
  print("Row 0: Xmin = %.5f, Xmax = %.5f, DetectorIDsList = %s." % (outmaskws.cell(0, 0), outmaskws.cell(0, 1), outmaskws.cell(0, 2)))

.. testcleanup:: ExHistSimple

   DeleteWorkspace(dataws)
   DeleteWorkspace(outmaskws)

Output:

.. testoutput:: ExHistSimple

  Number of rows:  1
  Row 0: Xmin = 12300.00000, Xmax = 24500.00000, DetectorIDsList =  27599-27698.

**Example - Extract mask to table with an optional input table workspace:**

.. testcode:: ExOptTable

  # load data
  dataws = LoadNexusProcessed(Filename="PG3_2538_2k.nxs")

  # mask some detectors
  for i in range(100):
      dataws.maskDetectors(100+i)

  # create a mask table workspacetws =
  tws = CreateEmptyTableWorkspace()
  tws.addColumn("double", "XMin")
  tws.addColumn("double", "XMax")
  tws.addColumn("str", "DetectorIDsList")
  tws.addRow([10000, 20000, "10000"])
  tws.addRow([12000, 20000, "20000, 20002-20004"])

  # run algorithm
  outmaskws = ExtractMaskToTable(InputWorkspace=dataws, MaskTableWorkspace=tws, Xmin = 12300., Xmax = 24500.)

  # Write some result
  print("Number of rows:  {}".format(outmaskws.rowCount()))
  print("Row 0: Xmin = %.5f, Xmax = %.5f, DetectorIDsList = %s." % (outmaskws.cell(0, 0), outmaskws.cell(0, 1), outmaskws.cell(0, 2)))
  print("Row 1: Xmin = %.5f, Xmax = %.5f, DetectorIDsList = %s." % (outmaskws.cell(1, 0), outmaskws.cell(1, 1), outmaskws.cell(1, 2)))
  print("Row 2: Xmin = %.5f, Xmax = %.5f, DetectorIDsList = %s." % (outmaskws.cell(2, 0), outmaskws.cell(2, 1), outmaskws.cell(2, 2)))

.. testcleanup:: ExOptTable

  DeleteWorkspace(dataws)
  DeleteWorkspace(outmaskws)
  DeleteWorkspace(tws)

Output:

.. testoutput:: ExOptTable

  Number of rows:  3
  Row 0: Xmin = 10000.00000, Xmax = 20000.00000, DetectorIDsList = 10000.
  Row 1: Xmin = 12000.00000, Xmax = 20000.00000, DetectorIDsList = 20000, 20002-20004.
  Row 2: Xmin = 12300.00000, Xmax = 24500.00000, DetectorIDsList =  27599-27698.

.. categories::

.. sourcelink::
