.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This workflow algorithm is to export a MatrixWorkspace to a GSAS data file, 
which is close enough to the GSAS file generated from VULCAN's V-drive. 

V-drive uses a different bin other than Mantid does.  
Besides, IDL's rebin algorithm is different from Mantid's. 


Usage
-----

**Example - Save GSAS file in V-drive style:**

.. testcode:: ExSaveVulcanGSAS

  import os
 
  # load data and create header
  dataws = Load(Filename="focussed.nxs")
  dataws.setTitle("Test")
  dataws = ConvertUnits(InputWorkspace=dataws, Target="TOF", EMode="Elastic", AlignBins=False)
  
  file_name = "testvulcan.gda"
  path = os.path.join(os.path.expanduser("~"), file_name)
  
  # create binning table
  ref_bin_table = CreateEmptyTableWorkspace(OutputWorkspace='ref_bin_table')
  ref_bin_table.addColumn('str', 'indexes')
  ref_bin_table.addColumn('str', 'params')

  ref_bin_table.addRow(['0, 2, 4', '1000, -0.001, 20000'])
  ref_bin_table.addRow(['1, 3, 5', '2000, -0.002, 30000'])
  
  gsaws = SaveVulcanGSS(InputWorkspace=dataws, BinningTable="ref_bin_table", GSSFilename=path, IPTS=1234, GSSParmFileName="mock.prm")
  
  gfile = open(path, "r")
  lines = gfile.readlines()
  gfile.close()

  print("[GSAS File Start]")
  for i in range(15):
      print(lines[i].rstrip())
  print("... ...")
  
.. testcleanup:: ExSaveVulcanGSAS

  file_name = "testvulcan.gda"
  path = os.path.join(os.path.expanduser("~"), file_name)
  os.remove(path)

  DeleteWorkspace(Workspace="dataws") 
  DeleteWorkspace(Workspace="gsaws") 

Output:

.. testoutput:: ExSaveVulcanGSAS

  [GSAS File Start]
  Test
  Instrument parameter file: mock.prm
  #IPTS: 1234
  #binned by: Mantid
  #GSAS file name: testvulcan.gda
  #GSAS IPARM file: mock.prm
  #Pulsestart:    0
  #Pulsestop:     0
  #
  # Total flight path 19.3695m, tth 9.14421deg, DIFC 780.587
  # Data for spectrum :0
  BANK 2 2997 2997 SLOG 1000.5 19987.7 0.0010000 0 FXYE
                  1000.5                 520.0                 22.80
                  1001.5                 516.7                 22.73
                  1002.5                 515.2                 22.70
  ... ...

.. categories::

.. sourcelink::
