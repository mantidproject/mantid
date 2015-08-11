:orphan:

.. testcode:: mwTest_Extending_Mantid_With_Python_Exercise_3_Solution[1]

   # The aim of this exercise is to write a small algorithm that wraps a small script that focuses some powder diffraction data.
   
   # Write an algorithm called <code>PowerDiffractionReduce</code>
   #  - The algorithm should have 3 properties:
   #  - Filename: A FileProperty for a TOF data file to load (ignore extensions)
   #  - CalFilename: A FileProperty for a cal file (ignore extensions)
   #  - OutputWorkspace: An output WorkspaceProperty to hold the final result.
   
   
   # The steps the algorithm should perform are
   #  - Use the <code>Load</code> algorithm to load the TOF data
   #  - Run <code>AlignDetectors</code> on the TOF data using the file given by the <code>CalFilename</code> property
   #  - Run <code>DiffractionFocussing</code> on the previous output & focus the data using the same cal file as the previous step (called a grouping file here)
   #  - Set the output from the <code>DiffractionFocussing</code> algorithm as the output of <code>PowerDiffractionReduce</code>.
   #  - Delete the temporary reference using DeleteWorkspace
   
   
   # To test the algorithm, execute the script that contains the algorithm to register it with Mantid. It will then show up in the list of algorithms. Use the following inputs:
   #  - Filename: ''HRP39182.RAW''
   #  - CalFilename: ''hrpd_new_072_01_corr.cal''
   #  - OutputWorkspace: ''focussed''
   
   from mantid.kernel import *
   from mantid.api import *
   
   import numpy as np
    
   # Class definition
   class DiffractionPowderReduce(PythonAlgorithm):
    
       def PyInit(self):
           # 2 input properties
           self.declareProperty(FileProperty(name="Filename", defaultValue="",action=FileAction.Load),
                                doc="TOF data filename")
           self.declareProperty(FileProperty(name="CalFilename", defaultValue="",action=FileAction.Load),
                                doc="TOF data filename")
   
           # 1 Output property
           self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", 
                                                  direction=Direction.Output))
   
       def PyExec(self):
           from mantid.simpleapi import Load, AlignDetectors, DiffractionFocussing
           
           # Load file to workspace
           _tmpws = Load(Filename=self.getPropertyValue("Filename"))
           
           # AlignDetectors
           calfile = self.getProperty("CalFilename").value
           _tmpws = AlignDetectors(InputWorkspace=_tmpws,CalibrationFile=calfile)
   
           # Focus
           _tmpws = DiffractionFocussing(InputWorkspace=_tmpws,GroupingFileName=calfile)
   
           # Store reference after algorithm has gone
           self.setProperty("OutputWorkspace", _tmpws)
           
           DeleteWorkspace(_tmpws)
   
   # Register algorithm with Mantid
   AlgorithmFactory.subscribe(DiffractionPowderReduce)


