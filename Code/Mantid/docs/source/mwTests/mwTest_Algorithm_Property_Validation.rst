:orphan:

.. testcode:: mwTest_Algorithm_Property_Validation[20]

   def PyInit(self):
       # Force the value to be positive or zero
       self.declareProperty("Parameter",-1.0,FloatBoundedValidator(lower=0))
   
       # Require the user to provide a non-empty string as input
       self.declareProperty("Prefix", "", StringMandatoryValidator()) 
   
       # Require the property to have one of the listed values
       self.declareProperty("ProcessOption","Full",
                            StringListValidator(["Full","QuickEstimate"]))


