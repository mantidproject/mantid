:orphan:

.. testcode:: mwTest_Declaring_Simple_Properties[10]

   def PyInit(self):
       self.declareProperty('InputValue', -1)


.. testcode:: mwTest_Declaring_Simple_Properties[20]

   def PyInit(self):
       # input direction specified explicitly
       self.declareProperty('InputValue', -1, direction=Direction.Input)
       # output direction specified explicitly
       self.declareProperty('OutputValue', -1, direction=Direction.Output)


.. testcode:: mwTest_Declaring_Simple_Properties[33]

   def PyInit(self):
       self.declareProperty('InputValue', -1, direction=Direction.Input, 
                            doc="A input value for this algorithm")


