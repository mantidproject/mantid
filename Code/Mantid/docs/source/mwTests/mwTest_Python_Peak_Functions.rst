:orphan:

.. testcode:: mwTest_Python_Peak_Functions[18]

   from mantid.api import *
   import math
   import numpy as np
   
   class PyGaussian(IPeakFunction):
       
       def init(self):
           self.declareParameter("Height")
           self.declareParameter("PeakCentre")
           self.declareParameter("Sigma")
           # We don't use any attributes in this example
   
   FunctionFactory.subscribe(PyGaussian) # Registration is identical


.. testcode:: mwTest_Python_Peak_Functions[45]

   def functionLocal(self, xvals):
       height = self.getParameterValue("Height")
       peak_centre = self.getParameterValue("PeakCentre")
       sigma = self.getParameterValue("Sigma")
       weight = math.pow(1./sigma,2);
   
       offset_sq=np.square(xvals-peak_centre)
       out=height*np.exp(-0.5*offset_sq*weight)
       return out
       
   def functionDerivLocal(self, xvals, jacobian):
       height = self.getParameterValue("Height")
       peak_centre = self.getParameterValue("PeakCentre")
       sigma = self.getParameterValue("Sigma")
       weight = math.pow(1./sigma,2)
         
       # X index
       i = 0
       for x in xvals:
           diff = x-peak_centre
           exp_term = math.exp(-0.5*diff*diff*weight)
           jacobian.set(i,0, exp_term)
           jacobian.set(i,1, diff*height*exp_term*weight)
           # derivative with respect to weight not sigma
           jacobian.set(i,2, -0.5*diff*diff*height*exp_term)
           i += 1


