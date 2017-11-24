from mantid.api import FunctionFactory
 
class FunctionWrapper(object):
  """ Wrapper class for Fitting Function 
  """
  def __init__ (self, name, **kwargs):
    """ 
    Called when creating an instance
    :param name: name of fitting function to create or 
    an Ifunction object to wrap.
    :param **kwargs: standard argument for __init__ function
    """
    if not isinstance(name, str):
       self.fun = name
    else:
       self.fun = FunctionFactory.createFunction(name)
       # Deal with attributes first
       for key in kwargs:
          if key == "attributes":
             atts = kwargs[key]
             for keya in atts:
               self.fun.setAttributeValue(keya, atts[keya])
          elif self.fun.hasAttribute(key):
             self.fun.setAttributeValue(key, kwargs[key])
           
       # Then deal with parameters        
       for key in kwargs:
          if key != "attributes" and not self.fun.hasAttribute(key):
             self.fun.setParameter(key, kwargs[key])
 
  def __getattr__(self, item):
      """
      __getattr__ invoked when attribute item not found in the instance,
      nor in the class, nor its superclasses.
      :param item: named attribute
      :return: attribute of self.fun instance, or any of the fitting
       parameters or function attributes
      """
      if 'fun' in self.__dict__:
          if hasattr(self.fun, item):
              return getattr(self.fun, item)
          else:
              return self.__getitem__(item)
 
  def __setattr__(self, key, value):
      """
      __setattr__ invoked when attribute key not found in the instance,
      nor in the class, nor its superclasses.
      Enables direct access to the attributes of self.fun instance, and
      also to its fitting parameters and function attributes
      :param key: named attribute 
      :param value: new value for the named attribute
      """
      if key == 'fun':
          self.__dict__['fun'] = value  # initialize self.fun
      elif 'fun' in self.__dict__ and hasattr(self.fun, key):
          setattr(self.fun, key, value)  # key is attribute of instance self.fun
      elif 'fun' in self.__dict__ and self.fun.hasAttribute(key):
          self.fun.setAttributeValue(key, value)  # key is function attribute
      elif 'fun' in self.__dict__ and self.fun.hasParameter(key):
          self.fun.setParameter(key, value)  # key is fitting parameter
      else:
          self.__dict__[key] = value  # initialize self.key
 
  def __getitem__ (self, name):
      """ Called from array-like access on RHS
          It should not be called directly.
          :param name: name that appears in the []
      """
      if type(name) == type('string') and self.fun.hasAttribute(name):
         return self.fun.getAttributeValue(name)
      else:
         return self.fun.getParameterValue(name)
       
  def __setitem__ (self, name, value):
      """ Called from array-like access on LHS
          It should not be called directly.
       
          :param name: name that appears in the []
          :param value: new value of this item
      """
      if type(name) == type('string') and self.fun.hasAttribute(name):
         self.fun.setAttributeValue(name, value)
      else:
         self.fun.setParameter(name, value)
       
  def __str__ (self):
      """ Return string giving contents of function.
          Used in unit tests.
      """
      return self.fun.__str__()
 
  def __add__ (self, other):
      """ Implement + operator for composite function
       
          :param other: functionWrapper to be added to self
      """
      sum = CompositeFunctionWrapper(self, other)
      if sum.pureAddition:
         sum = sum.flatten()
      return sum    
         
  def __mul__ (self, other):
      """ Implement * operator for product function
       
          :param other: functionWrapper to multiply self by
      """
      prod = ProductFunctionWrapper(self, other)
      if prod.pureMultiplication:
         prod = prod.flatten()
      return prod
         
  def tie (self, *args, **kwargs):
    """ Add ties.
        :param *args: one or more dictionaries of ties
        :param **kwargs: one or more ties
    """
    for a in args:
        if isinstance(a, dict):
          for key in a:
            self.fun.tie(key, str(a[key]))
 
    for key in kwargs:
        self.fun.tie(key, str(kwargs[key]))
         
  def fix(self, name):
      """ Fix a parameter.
       
          :param name: name of parameter to be fixed
      """
      self.fun.fixParameter(name)
       
  def fixAllParameters(self):
       """ Fix all parameters.
       """
       for i in range(0, self.fun.numParams()):
          self.fix(self.getParameterName(i))
       
  def untie(self, name):
      """ Remove tie from parameter.
       
          :param name: name of parameter to be untied
      """
      self.fun.removeTie(name)
       
  def untieAllParameters(self):
      """ Remove ties from all parameters.
      """
      for i in range(0, self.fun.numParams()):  
          self.fun.removeTie(self.getParameterName(i))  
 
           
  def constrain(self, expressions):
      """ Add constraints
       
          :param expressions: string of tie expressions      
      """
      self.fun.addConstraints( expressions )
       
  def unconstrain(self, name):
      """ Remove constraints from a parameter
       
          :param name: name of parameter to be unconstrained
      """
      self.fun.removeConstraint(name)
            
  def free(self, name):
      """ Free a parameter from tie or constraint
       
          :param name: name of parameter to be freed
      """
      self.fun.removeTie(name)
      self.fun.removeConstraint(name)
       
  def getParameterName(self, index):
      """ Get the name of the parameter of given index
       
          :param index: index of parameter
      """
      return self.fun.getParamName(index)
        
  @property
  def function(self):
      """ Return the underlying IFunction object
      """      
      return self.fun
 
  @property
  def name(self):
      """ Return the name of the function
      """
      return self.fun.name()
       
class CompositeFunctionWrapper(FunctionWrapper):
    """ Wrapper class for Composite Fitting Function
    """
    def __init__ (self, *args):
       """ Called when creating an instance
           It should not be called directly
           :param *args: names of functions in composite function
       """
       self.pureAddition = True
       self.pureMultiplication = False
       return self.initByName("CompositeFunction", *args)
 
    def initByName(self, name, *args):
       """ intialise composite function of named type.
           E.g. "ProductFunction"
           This function would be protected in c++
           and should not be called directly except
           by __init__ functions of this class and
           subclasses.
            
          :param name: name of class calling this.
          :param *args: names of functions in composite function
       """
       if len(args) == 1 and  not isinstance(args[0], FunctionWrapper):
          # We have a composite function to wrap
          self.fun = args[0]
       else:
          self.fun = FunctionFactory.createCompositeFunction(name)
    
          #Add the functions, checking for Composite & Product functions
          for a in args:
             if not isinstance(a, int):
                if isinstance(a, CompositeFunctionWrapper):
                   if self.pureAddition:
                      self.pureAddition = a.pureAddition
                   if self.pureMultiplication:
                      self.pureMultiplication = a.pureMultiplication
                functionToAdd = FunctionFactory.createInitialized( a.fun.__str__() )             
                self.fun.add(functionToAdd)    
       
    def getParameter(self, name):
        """ get value of parameter of specified name
       
            :param name: name of parameter
        """
        return self.fun.getParameterValue(name)
         
    def getCompositeParameterName(self, name, index):
        """ get composite parameter name of parameter of 
            given name of member function of given index
        """
        return "f"+str(index)+"."+name
         
    def getIndexOfFunction (self, name):
        """ get index of function specified by name,
            such as "LinearBackground" for the only
            LinearBackground function or
            "Gaussian1" for the second Gaussian function.
       
            :param name: name specifying the function
        """
        # Only a shallow search is done.
         
        delimiter = " "
        if name.count(delimiter) == 0:
           fname = name
           occurrence = 0
        else:
           fname, n = name.split(delimiter)
           occurrence = int(n)
            
        index = 0
        count = 0
        for f in self:
           if f.fun.name() == fname:
              if( count == occurrence):
                 return index
              else:
                 count += 1
           index += 1
          
        raise RuntimeError("Specified function not found.")
         
    def f (self, name):
        """ get function specified by name,
            such as "LinearBackground" for the only
            LinearBackground function or
            "Gaussian1" for the second Gaussian function.
       
            :param name: name specifying the function
        """
        index = self.getIndexOfFunction(name)
        return self[index]
 
    def __getitem__ (self, nameorindex):
        """ get function of specified index or parameter of specified name
            called for array-like access on RHS.
            It should not be called directly.
       
            :param name: name or index in the []
        """
 
        comp = self.fun
        item = comp[nameorindex]
        if isinstance(item, float):
           return  item
        elif item.name() == "CompositeFunction":
           return CompositeFunctionWrapper(item)
        elif item.name() == "ProductFunction":
           return ProductFunctionWrapper(item)
        elif item.name() == "Convolution":
           return ConvolutionWrapper(item)
        else:       
           return FunctionWrapper(item)
 
    def __setitem__ (self, name, newValue):
        """ Called from array-like access on LHS
            It should not be called directly.
       
            :param name: name or index in the []
            :param newValue: new value for item
        """
        comp = self.fun
        if isinstance( newValue, FunctionWrapper):
           comp[name] = newValue.fun
        else:
           comp[name] = newValue
                     
    def __iadd__ (self, other):
       """ Implement += operator.
           It should not be called directly.
            
           :param other: object to add
       """
       self.fun.add(other.fun)
       return self
        
    def __delitem__ (self, index):
       """ Delete item of given index from composite function.
           It should not be called directly.
            
           :param index: index of item
       """
       self.fun.__delitem__(index)
        
    def __len__ (self):
       """ Return number of items in composite function.
           Implement len() function.
           It should not be called directly.
       """
 
       composite = self.fun
       return composite.__len__()

        
    def tieAll (self, name):
       """ For each member function, tie the parameter of the given name 
           to the parameter of that name in the first member function.
           The named parameter must occur in all the member functions.
            
           :param name: name of parameter
       """
       expr = self.getCompositeParameterName(name, 0)
       self.tie({self.getCompositeParameterName(name, i): expr for i in range(1,len(self)) }) 
           
    def fixAll (self, name):
       """ Fix all parameters with the given local name.
           Every member function must have a parameter of this name.
            
           :param name: name of parameter
       """
       for f in self:
         f.fix(name)
 
           
    def constrainAll (self, expressions):
       """ Constrain all parameters according local names in expressions.
                   
           :param expressions: string of expressions
       """
       for i in range(0, len(self)):
          if isinstance( self[i], CompositeFunctionWrapper ):
             self[i].constrainAll(expressions)
          else:
             try:
                self[i].constrain(expressions)
             except:
                pass
           
    def unconstrainAll (self, name):
       """ Unconstrain all parameters of given local name.
            
           :param name: local name of parameter
       """
       for i in range(0, len(self)):
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
            
           :param name: local name of parameter
       """
       for i in range(0, len(self)):
          self.untie(self.getCompositeParameterName(name, i))
 
    def flatten (self):
       """ Return composite function, equal to self, but with 
           every composite function within replaced by
           its list of functions, so having a pure list of functions.
           This makes it possible to index and iterate all the functions
           and use tieAll() and untieAll().
           Not to be used with a mixture of product and sum 
           composite functions, because the arithmetic 
           may no longer be correct.
           The return value is not independent of self.
       """
       # If there are no composite functions, do nothing
       needToFlatten = False
       for i in range(0, len(self)):
          if not needToFlatten and isinstance(self[i],CompositeFunctionWrapper):
             needToFlatten = True
              
       if not needToFlatten :
          return self
        
       # Now we know there is a composite function.
       if isinstance(self,ProductFunctionWrapper):
          flatSelf = ProductFunctionWrapper()
       else:
          flatSelf = CompositeFunctionWrapper()
           
       for i in range(0, len(self)):
          if isinstance(self[i],CompositeFunctionWrapper):
             currentFunction = self[i].flatten()
             for j in range(0, len(currentFunction)):
                flatSelf.fun.add(currentFunction[j].fun)
          else:
             flatSelf.fun.add(self[i].fun)
              
       return flatSelf          
  
class ProductFunctionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Product Fitting Function
    """
    def __init__ (self, *args):
       """ Called when creating an instance
           It should not be called directly.
           :param *args: names of functions in composite function
       """
       self.pureAddition = False
       self.pureMultiplication = True
       return self.initByName("ProductFunction", *args)
        
class ConvolutionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Convolution Fitting Function
    """
    def __init__ (self, *args):
       """ Called when creating an instance
           It should not be called directly.
           :param *args: names of functions in composite function
       """
       self.pureAddition = False
       self.pureMultiplication = False
       return self.initByName("Convolution", *args)
        
class MultiDomainFunctionWrapper(CompositeFunctionWrapper):
    """ Wrapper class for Product Fitting Function
    """
    def __init__ (self, *args, **kwargs):
       """ Called when creating an instance
           It should not be called directly
           :param *args: names of functions in composite function
       """
       # Assume it's not safe to flatten
       self.pureAddition = False
       self.pureMultiplication = False
        
       # Create and populate with copied functions
       self.initByName("MultiDomainFunction", *args)
                
       # Tie the global parameters
       if 'Global' in kwargs:
          list = kwargs['Global']
          for name in list:
             self.tieAll(name)
        
       # Set domain indices: 1 to 1       
       for i in range(0, len(self)):
          self.fun.setDomainIndex(i, i)
     
    @property     
    def nDomains (self):
       """ Return number of domains 
       """
       return self.fun.nDomains()
      
 
def _create_wrapper_function(name):
    """Create fake functions for the given name
       It should not be called directly
                   
       :param name: name of fake function
    """
    # ------------------------------------------------------------------------------------------------
    def wrapper_function(*args, **kwargs):
        name_to_constructor = {
            'CompositeFunction': CompositeFunctionWrapper,
            'ProductFunction': ProductFunctionWrapper,
            'Convolution': ConvolutionWrapper,
            'MultiDomainFunction': MultiDomainFunctionWrapper,
            }
        # constructor is FunctionWrapper if the name is not in the registry.
        constructor = name_to_constructor.get(name, FunctionWrapper)  
        return  constructor(name, *args, **kwargs)
 
    # ------------------------------------------------------------------------------------------------
    wrapper_function.__name__ = name
    # _replace_signature(fake_function, ("", ""))
    globals()[name] = wrapper_function
 
fnames = FunctionFactory.getFunctionNames()
for i, val in enumerate(fnames):
   _create_wrapper_function(val)
