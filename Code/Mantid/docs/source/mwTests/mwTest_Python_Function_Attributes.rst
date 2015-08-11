:orphan:

.. testsetup:: mwTest_Python_Function_Attributes[10]

   from mantid.kernel import *
   from mantid.api import *

.. testcode:: mwTest_Python_Function_Attributes[10]

   # Remember to choose either IFunction1D or IPeakFunction 
   class Example1DFunction(IFunction1D): # or IPeakFunction
   
       def init(self):
           self.declareParameter("A0", 0.0)
           self.declareParameter("A1", 0.0)
           
           self.declareAttribute("NLoops", 10)


.. testsetup:: mwTest_Python_Function_Attributes[34]

   from mantid.kernel import *
   from mantid.api import *

.. testcode:: mwTest_Python_Function_Attributes[34]

   class Example1DFunction(IFunction1D):
   
     def init(self):
       self.declareParameter("A0", 0.0)
       self.declareParameter("A1", 0.0)
           
       self.declareAttribute("NLoops", 10)
   
     def setAttributeValue(self, name, value):
       if name == "NLoops":
         # Can the be accessed quicker later using self._nloops
         self._nloops = value


