:orphan:

.. testcode:: mwTest_Workspace_Validators[17]

   def PyInit(self):
     # Requires the input workspace to have x-axis units of Wavelength
     self.declareProperty(WorkspaceProperty(name="InputWorkspace", 
                                            defaultValue="", 
                                            direction=Direction.Input, 
                                            validator=WorkspaceUnitValidator("Wavelength")))


