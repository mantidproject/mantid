from mantid.simpleapi import FunctionFactory

class FunctionWrapper:
  """ Wrapper class for Fitting Function 
  """
  def __init__ (self, name, **kwargs):
    """ Called when creating an instance
    """
    if not isinstance(name, str):
       self.fun = name
    else:
       self.fun = FunctionFactory.createFunction(name)
    
       # Deal with attributes first
#       for key in kwargs:
#          if self.fun.hasAttribute(key):
#              self.fun.storeAttributeValue(key, kwargs[key])
            
       # Then deal with parameters        
       for key in kwargs:
          self.fun.setParameter(key, kwargs[key])
             
  def __getitem__ (self, name):
      """ Called from array-like access on RHS
      """
      return self.fun.getParameterValue(name)
      
  def __setitem__ (self, name, value):
      """ Called from array-like access on LHS
      """
      self.fun.setParameter(name, value)
      
  def __str__ (self):
      """ Return string giving contents of function.
          Used in unit tests.
      """
      return self.fun.__str__()
                 
  def __add__ (self, other):
      """ Implement + operator for composite function
      """
      if isinstance(self,CompositeFunctionWrapper):
        self.fun.add(other.fun)
        return self.flatten()
      else:
        sum = CompositeFunctionWrapper(self,other)
        return sum.flatten()
        
  def __mul__ (self, other):
      """ Implement * operator for product function
      """
      if isinstance(self,ProductFunctionWrapper):
        self.fun.add(other.fun)
        return self.flatten()
      else:
        product = ProductFunctionWrapper(self,other)
        return product.flatten()
        
  def tie (self, *args, **kwargs):
    """ Add ties.
    """
    for a in args:
        if isinstance(a, dict):
          for key in a:
            self.fun.tie(key, str(a[key]), False)

    for key in kwargs:
        self.fun.tie(key, str(kwargs[key]), False)
        
  def fix(self, name):
      """ Fix a parameter.
      """
      self.fun.fix(name)
      
  def fixAllParameters(self):
       """ Fix all parameters.
       """
       for i in range(0, self.fun.numParams()):
          self.fix(self.getParameterName(i))
      
  def untie(self, name):
      """ Remove tie from parameter.
      """
      self.fun.untie(name)
      
  def untieAllParameters(self):
      """ Remove ties from all parameters.
      """
      for i in range(0, self.fun.numParams()):
          self.untie(self.getParameterName(i))
          
  def constrain(self, expressions):
      """ Add constraints 
      """
      self.fun.constrain( expressions, False )
      
  def unconstrain(self, name):
      """ Remove constraints from a parameter
      """
      self.fun.unconstrain(name)
           
  def free(self, name):
      """ Free a parameter from tie or constraint
      """
      self.fun.untie(name)
      self.fun.unconstrain(name)
      
  def getParameterName(self, index):
      """ Get the name of the parameter of given index
      """
      return self.fun.getParamName(index)
      
  def getFunction(self):
      """ Return the underlying IFunction object
      """
      return self.fun

      
class CompositeFunctionWrapper(FunctionWrapper):
    """ Wrapper class for Composite Fitting Function
    """
    def __init__ (self, *args):
       """ Called when creating an instance
       """
       return self.initByName("CompositeFunction", *args)

    def initByName(self, name, *args):
       """ intialise composite function of named type.
           E.g. "ProductFunction"
       """
       if len(args) == 1 and hasattr(args[0],'nFunctions'):
          # We have a composite function to wrap
          self.fun = args[0]
       else:
          if name == "CompositeFunction":
             self.fun = FunctionFactory.createFunction(name)
          else:
             self.fun = FunctionFactory.createCompositeFunction(name)
   
          #Add the functions
          for a in args:
             if not isinstance(a, int): 
                self.fun.add(a.fun)    
      
    def getParameter(self, name):
        """ get parameter of specified name
        """
        return self.fun.getParameterValue(name)
        
    def getCompositeParameterName(self, name, index):
        """ get composite parameter name of parameter of 
            given name of member function of given index
        """
        return "f"+str(index)+"."+name

    def __getitem__ (self, nameorindex):
        """ get function of specified index or parameter of specified name
            called for array-like access on RHS.
        """
        item = self.fun.__getitem__(nameorindex)
        if isinstance(item, float):
           return  item
        elif hasattr(item, 'nFunctions'):
           # item is a CompositeFunction
           return CompositeFunctionWrapper(item)
        else:           
           return FunctionWrapper(item)

    def __setitem__ (self, name, newValue):
        """ Called from array-like access on LHS
        """
        self.fun.__setitem__(name, newValue)
                    
    def __iadd__ (self, other):
       """ Implement += operator.
       """
       self.fun.add(other.fun)
       return self
       
    def __delitem__ (self, index):
       """ Delete item of given index from composite function.
       """
       self.fun.__delitem__(index)
       
    def __len__ (self):
       """ Return number of items in composite function.
           Implement len() function.
       """
       return self.fun.__len__()
       
    def __iter__ (self):
       """ Implement iteration
       """
       return self
       
    def tieAll (self, name):
       """ For each member function, tie the parameter of the given name 
           to the parameter of that name in the first member function.
           The named parameter must occur in all the member functions.
       """
       expr = self.getCompositeParameterName(name, 0)
       self.tie({self.getCompositeParameterName(name, i): expr for i in range(1,self.__len__()) }) 
          
    def fixAll (self, name):
       """ Fix all parameters with the given local name.
           Every member function must have a parameter of this name.
       """
       for i in range(0, self.__len__()):
          self[i].fix(name)
          
    def constrainAll (self, expressions):
       """ Constrain all parameters according local names in expressions.
       """
       for i in range(0, self.__len__()):
          if isinstance( self[i], CompositeFunctionWrapper ):
             self[i].constrainAll(expressions)
          else:
             try:
                self[i].constrain(expressions)
             except:
                pass
          
    def unconstrainAll (self, name):
       """ Constrain all parameters according local names in expressions.
       """
       for i in range(0, self.__len__()):
          if isinstance( self[i], CompositeFunctionWrapper ):
             self[i].unconstrainAll(name)
          else:
             try:
                self[i].unconstrain(name)
             except:
                pass
          
    def untieAll (self, name):
       """ Untie all parameters with the given local name.
           Every member function must have a parameter of this name.
       """
       for i in range(0, self.__len__()):
          self[i].untie(name)

    def flatten (self):
       """ Return composite function, equal to self, but with 
           every composite function within replaced by
           its list of functions, so having a pure list of functions.
           This enables sensible __add__ and __mult__ operations.
       """
       # If there are no composite functions, do nothing
       needToFlatten = False
       for i in range(0, self.__len__()):
          if not needToFlatten and isinstance(self[i],CompositeFunctionWrapper):
             needToFlatten = True
             
       if not needToFlatten :
          return self
       
       flatSelf = CompositeFunctionWrapper()
       for i in range(0, self.__len__()):
          if isinstance(self[i],CompositeFunctionWrapper):
             currentFunction = self[i].flatten()
             for j in range(0, currentFunction.__len__()):
                flatSelf.fun.add(currentFunction[j].fun)
          else:
             flatSelf.fun.add(self[i].fun)
             
       return flatSelf          
 
class ProductFunctionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Product Fitting Function
    """
    def __init__ (self, *args):
       return self.initByName("ProductFunction", *args)
       
class ConvolutionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Convolution Fitting Function
    """
    def __init__ (self, *args):
       return self.initByName("Convolution", *args)
       
class MultiDomainFunctionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Product Fitting Function
    """
    def __init__ (self, *args):
       self.initByName("MultiDomainFunction", *args)
       for i in range(0, self.__len__()):
          self.fun.setDomainIndex(i, i)
          
    def nDomains (self):
       """ Return number of domains 
           Used for testing
       """
       return self.fun.nDomains()
     
        
def _create_wrapper_function(name):
    """Create fake functions for the given name
    """
    # ------------------------------------------------------------------------------------------------
    def wrapper_function(*args, **kwargs):
        if  name == "CompositeFunction":
           return CompositeFunctionWrapper( *args, **kwargs )
        elif name == "ProductFunction":
           return ProductFunctionWrapper( *args, **kwargs )
        elif name == "Convolution":
           return ConvolutionWrapper( *args, **kwargs )
        elif name == "MultiDomainFunction":
           return MultiDomainFunctionWrapper( *args, **kwargs )
        else:
           return FunctionWrapper(name, *args, **kwargs)

    # ------------------------------------------------------------------------------------------------
    wrapper_function.__name__ = name
    # _replace_signature(fake_function, ("", ""))
    globals()[name] = wrapper_function

fnames = FunctionFactory.getFunctionNames()
for i, val in enumerate(fnames):    
   _create_wrapper_function(val)

