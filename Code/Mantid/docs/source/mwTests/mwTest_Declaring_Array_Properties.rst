:orphan:

.. testcode:: mwTest_Declaring_Array_Properties[13]

   def PyInit(self):
     self.declareProperty(FloatArrayProperty("Floats",
                                             direction=Direction.Input), 
                                             doc='Input doubles')
     self.declareProperty(IntArrayProperty("Ints",
                                             direction=Direction.Input),
                                             doc='Input integers')
     self.declareProperty(StringArrayProperty("Strings",
                                             direction=Direction.Input),
                                             doc='Input strings')


.. testcode:: mwTest_Declaring_Array_Properties[29]

   def PyInit(self):
     self.declareProperty(FloatArrayProperty(name="PythonListInput",
                                             values=[1.2,4.5,6.7],
                                             direction=Direction.Input))


.. testcode:: mwTest_Declaring_Array_Properties[49]

   def PyInit(self):
      length_validator = FloatArrayLengthValidator(5)
      self.declareProperty(FloatArrayProperty("Floats", 
                                             validator=length_validator,
                                             direction=Direction.Input))


