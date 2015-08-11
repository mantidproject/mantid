:orphan:

.. testcode:: mwTest_Declaring_Workspace_Properties[7]

   from mantid.kernel import *
   from mantid.api import *
   
   class WorkspaceProperties(PythonAlgorithm):            
     
     def PyInit(self):
       self.declareProperty(WorkspaceProperty(name="InputWorkspace", 
                                              defaultValue="", 
                                              direction=Direction.Input), 
                            doc="Documentation for this property")
               
     def PyExec(self):
         pass


.. testcode:: mwTest_Declaring_Workspace_Properties[33]

   from mantid.kernel import *
   from mantid.api import *
   
   class MatrixWorkspaceProperties(PythonAlgorithm):            
     
     def PyInit(self):
         self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", 
                                                      "", 
                                                      Direction.Input), 
                              "Documentation for this property")
               
     def PyExec(self):
         pass


