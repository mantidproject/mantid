from mantid.simpleapi import FunctionFactory

class FunctionWrapper:
# Wrapper class for Fitting Function 
  def __init__ (self, name, **kwargs):
    self.fun = FunctionFactory.createFunction(name)
    
    # Deal with attributes first
#    for key in kwargs:
#       if(self.fun.hasAttribute(key)):
#           self.fun.storeAttributeValue(key, kwargs[key])
            
    # Then deal with parameters        
    for key in kwargs:
       #print key, "corresponds to", kwargs[key]
       self.fun.setParameter(key, kwargs[key])
       
  def __getitem__ (self, name):
      return self.fun.getParameterValue(name)
      
  def __setitem__ (self, name, value):
      self.fun.setParameter(name, value)
        
  def getFunction(self):
      return self.fun
      
class CompositeFunctionWrapper(FunctionWrapper):
# Wrapper class for Composite Fitting Function
    def __init__ (self, *args):
        self.fun = FunctionFactory.createFunction("CompositeFunction")
   
        #Search for Function arguments, if found create a FunctionWrapper for it.
        for a in args:
           self.fun.add(a.fun)
      
    def __getitem__ (self, name):
    # Override
        pass

    def __getitem__ (self, index):
    # get function of specified index
        pass

    def __setitem__ (self, name):
        pass
      
      