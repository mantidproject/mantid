:orphan:

.. testcode:: mwTest_Python_1D_Functions[7]

   from mantid.api import *
   import numpy
   
   class Example1DFunction(IFunction1D):
   
       def init(self):
           self.declareParameter("C", 0.0)
   
       def function1D(self, xvals):
           # Access current values during the fit
           c = self.getParameterValue("C")
   
           return c*numpy.sqrt(xvals)
   
   FunctionFactory.subscribe(Example1DFunction)


