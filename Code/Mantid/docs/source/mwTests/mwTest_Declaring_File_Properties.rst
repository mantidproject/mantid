:orphan:

.. testcode:: mwTest_Declaring_File_Properties[10]

   def PyInit(self):
       self.declareProperty(FileProperty(name="InputFile",defaultValue="",
                                         action=FileAction.Load, 
                                         extensions = ["txt"]))


