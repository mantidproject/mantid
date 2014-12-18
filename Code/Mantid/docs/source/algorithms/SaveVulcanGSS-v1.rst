.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is to export a MatrixWorkspace to a GSAS data file, 
which is close enough to the GSAS file generated from VULCAN's V-drive. 

V-drive uses a different bin other than Mantid does.  
Besides, IDL's rebin algorithm is different from Mantid's. 


Usage
-----

**Example - Save GSAS file in V-drive style:**

.. testcode:: ExSaveVulcanGSAS

  import os
  
  dataws = Load(Filename="focussed.nxs")
  dataws.setTitle("Test")
  dataws = ConvertUnits(InputWorkspace=dataws, Target="TOF", EMode="Elastic", AlignBins=False)
  
  file_name = "testvulcan.gda"
  path = os.path.join(os.path.expanduser("~"), file_name)
  
  gsaws = SaveVulcanGSS(InputWorkspace=dataws, BinFilename="vdrive_log_bin.dat", GSSFilename=path, IPTS=1234, GSSParmFileName="mock.prm")
  
  gfile = open(path, "r")
  lines = gfile.readlines()
  gfile.close()

  print "[GSAS File Start]"
  for i in xrange(11):
      print lines[i].rstrip()
  print "... ..."

  
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
  BANK 2 2487 2487 SLOG 5000.0 59998.0 0.0010005 0 FXYE        
        5000.0       531.6       23.06
        5005.0       530.0       23.02
  ... ...

.. categories::
