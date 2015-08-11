:orphan:

.. testcode:: mwTest_Combining_Validators[4]

   def PyInit(self):
     validation = CompositeValidator()
     validation.add(WorkspaceUnitValidator("Wavelength"))
     validation.add(InstrumentValidator())
   
     # or create validator from list
     # validation = CompositeValidator([
     #                  WorkspaceUnitValidator("Wavelength"), 
     #                  InstrumentValidator()
     #              ])
     self.declareProperty(WorkspaceProperty("InputWorkspace",
                                            "",
                                            Direction.Input, 
                                            validation))


