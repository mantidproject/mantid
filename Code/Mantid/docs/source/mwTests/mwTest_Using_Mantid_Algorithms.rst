:orphan:

.. testcode:: mwTest_Using_Mantid_Algorithms[7]

   from mantid.kernel import *
   from mantid.api import *
   
   class LoadAndDoSomething(PythonAlgorithm):
    
       def PyInit(self):
           self.declareProperty(FileProperty("Filename", "",
                                             action=FileAction.Load))
           self.declareProperty(MatrixWorkspaceProperty(
                                       "OutputWorkspace",
                                       "",
                                       direction=Direction.Output))
           # ... other stuff
    
       def PyExec(self):
           from mantid.simpleapi import Load, Scale, DeleteWorkspace
   	
           _tmpws = Load(Filename=self.getPropertyValue("Filename"))
           _tmpws = Scale(InputWorkspace=_tmpws,Factor=100)
   
           # Sets reference externally and sets the name to that 
           # given by the OutputWorkspace property
           self.setProperty("OutputWorkspace", _tmpws)
   
           # Removes temporary reference created here
           # (doesn't delete workspace) 
           DeleteWorkspace(_tmpws)


