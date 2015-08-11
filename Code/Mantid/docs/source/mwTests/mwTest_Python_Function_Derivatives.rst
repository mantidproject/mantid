:orphan:

.. testsetup:: mwTest_Python_Function_Derivatives[10]

   from mantid.kernel import *
   from mantid.api import *

.. testcode:: mwTest_Python_Function_Derivatives[10]

   class Example1DFunction(IFunction1D):
   
       def init(self):
           self.declareParameter("A0", 0.0)
           self.declareParameter("A1", 0.0)
   
       def function1D(self, xvals):
           a0 = self.getParameterValue("A0")
           a1 = self.getParameterValue("A1")
   
           # Use numpy arithmetic to compute new array
           return a0 + a1*xvals
   
       def functionDeriv1D(self, xvals, jacobian):
           i = 0
           for x in xvals:
             jacobian.set(i,0,1) # paramter at index 0
             jacobian.set(i,1,x) # paramter at index 1
             i += 1 
   
   FunctionFactory.subscribe(Example1DFunction)


