:orphan:

.. testcode:: mwTest_Python_Function_Active_Parameters[38]

   from mantid.api import *
   import math
   import numpy as np
   
   class PyGaussian(IFunction1D):
       
     def init(self):
         self.declareParameter("Height")
         self.declareParameter("PeakCentre")
         # Parameter is declared as you want it to be seen by the user
         self.declareParameter("Sigma")
          
     def function1D(self, xvals):
       # Can also be retrieve by index self.getParameterValue(0)
       height = self.getParameterValue("Height") 
       peak_centre = self.getParameterValue("PeakCentre")
       sigma = self.getParameterValue("Sigma")
       weight = math.pow(1./sigma,2)
         
       offset_sq = np.square(xvals-peak_centre)
       out = height*np.exp(-0.5*offset_sq*weight)
       return out
       
     def activeParameter(self, index):
       # Return the value of the parameter at the given index 
       # (ordered by the order in init)
       param_value = self.getParameterValue(index)
   
       if index == 2: #Sigma. Actually fit to 1/(sigma^2) for stability
         # param_value contains value of sigma
         return 1./math.pow(param_value,2) 
       else:
         # Deal with other cases. In this case we just want value as is
         return param_value 
   
     def setActiveParameter(self, index, value):
       # The framework minimizer wants to update the value of the parameter
       param_value = value
       explicit = False
       if index == 2: #sigma parameter index
         # value passed in is actually 1/sigma^2 so we need to translate
         # back to sigma
         param_value = math.sqrt(math.fabs(1.0/value)) 
       else:
         param_value = value # others are 1:1
         
       # Finally, actually update the values stored in function
       # so that the next call to function1D sees them
       self.setParameter(index, param_value, False)


