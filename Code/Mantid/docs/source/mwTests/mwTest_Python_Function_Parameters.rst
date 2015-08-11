:orphan:

.. testcode:: mwTest_Python_Function_Parameters[10]

   from mantid.api import *
   
   # Remember to choose either IFunction1D or IPeakFunction 
   class Example1DFunction(IFunction1D): # or IPeakFunction 
   
       def init(self):
           # Parameter with default value 1.0
           self.declareParameter("Constant", 1.0)


