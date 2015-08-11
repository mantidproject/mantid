:orphan:

.. Skipping Test  mwTest_PythonAlgorithm_Body[7]


.. testcode:: mwTest_PythonAlgorithm_Body[14]

   def PyExec(self):
       sum = 0
       for i in range(100):
           sum += 1


.. testcode:: mwTest_PythonAlgorithm_Body[30]

   def PyInit(self):
       self.declareProperty("NIterations", 100, 
                            IntBoundedValidator(lower=0), 
                            "The number of iterations of the loop to "
                            "perform (default=100)")
   
   def PyExec(self):
       nloops = self.getProperty("NIterations").value
       sum = 0
       for i in range(nloops):
           sum += 1


.. testcode:: mwTest_PythonAlgorithm_Body[55]

   def PyInit(self):
       self.declareProperty(name="NIterations", defaultValue=100, 
                            validator=IntBoundedValidator(lower=0), 
                            doc="The number of iterations of the loop "
                                "to perform (default=100)")
       self.declareProperty(name="SummedValue", defaultValue=0, 
                            doc="Outputs the sum of the n iterations", 
                            direction = Direction.Output)
   
   def PyExec(self):
       nloops = self.getProperty("NIterations").value
       sum = 0
       for i in range(nloops):
           sum += 1
   
       self.setProperty("SummedValue", sum)


