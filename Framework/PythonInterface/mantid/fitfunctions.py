# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import FunctionFactory, Workspace, AlgorithmManager, IFunction1D
import numpy as np


class FunctionWrapper(object):
    """
    Wrapper class for Fitting Function
    """

    @staticmethod
    def wrap(fun, *args, **kwargs):
        fun_is_str = isinstance(fun, str)
        name = fun if fun_is_str else fun.name()
        if name in _name_to_constructor_map:
            wrapper = _name_to_constructor_map[name]
            if fun_is_str:
                return wrapper(*args, **kwargs)
            else:
                return wrapper(fun, *args, **kwargs)
        return FunctionWrapper(fun, **kwargs)

    def __init__(self, name, params_to_optimize=None, **kwargs):
        """
        Called when creating an instance

        :param name:   name of fitting function to create or an Ifunction object to wrap.
        :param params_to_optimize:   names of the parameters you want to optimize. The omitted parameters will remain
                       fixed at their initial value.
        :param kwargs: standard argument for initializing fit function
        """
        if not isinstance(name, str):
            self.fun = name
        else:
            self.fun = FunctionFactory.createFunction(name)
        self.init_paramgeters_and_attributes(**kwargs)

        def set_parameters_by_index(params):
            for i in range(len(params)):
                self.fun.setParameter(i, params[i])

        def set_parameters_by_name(params):
            for i, param_name in enumerate(params_to_optimize):
                self.fun.setParameter(param_name, params[i])

        self._set_parameters = set_parameters_by_index if params_to_optimize is None else set_parameters_by_name

    def init_paramgeters_and_attributes(self, **kwargs):
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
        if "fun" in self.__dict__:
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
        if key == "fun":
            self.__dict__["fun"] = value  # initialize self.fun
        elif "fun" in self.__dict__ and hasattr(self.fun, key):
            setattr(self.fun, key, value)  # key is attribute of instance self.fun
        elif "fun" in self.__dict__ and self.fun.hasAttribute(key):
            self.fun.setAttributeValue(key, value)  # key is function attribute
        elif "fun" in self.__dict__ and self.fun.hasParameter(key):
            self.fun.setParameter(key, value)  # key is fitting parameter
        else:
            self.__dict__[key] = value  # initialize self.key

    def __getitem__(self, name):
        """
        Called from array-like access on RHS

        **It should not be called directly.**

        :param name: name that appears in the []
        """
        if isinstance(name, str) and self.fun.hasAttribute(name):
            return self.fun.getAttributeValue(name)
        else:
            if self.fun.hasParameter(name):
                return self.fun.getParameterValue(name)
            else:
                raise AttributeError("Parameter %s not found" % name)

    def __setitem__(self, name, value):
        """
        Called from array-like access on LHS

        **It should not be called directly.**

        :param name: name that appears in the []
        :param value: new value of this item
        """
        if isinstance(name, str) and self.fun.hasAttribute(name):
            self.fun.setAttributeValue(name, value)
        else:
            self.fun.setParameter(name, value)

    def __str__(self):
        """
        Return string giving contents of function.
        Used in unit tests.
        """
        return str(self.fun)

    def __add__(self, other):
        """
        Implement + operator for composite function

        :param other: functionWrapper to be added to self
        """
        sum = CompositeFunctionWrapper(self, other)
        if sum.pureAddition:
            sum = sum.flatten()
        return sum

    def __mul__(self, other):
        """
        Implement * operator for product function

        :param other: functionWrapper to multiply self by
        """
        prod = ProductFunctionWrapper(self, other)
        if prod.pureMultiplication:
            prod = prod.flatten()
        return prod

    def __call__(self, x, *params):
        """
        Implement function evaluation, such that
        func(args) is equivalent func.__call__(args)

        :param x:      x value or list of x values
        :param params: list of parameter values
        """
        if isinstance(x, Workspace):
            # If the input is a workspace, simply return the output workspace.
            return self._execute_algorithm("EvaluateFunction", Function=self.fun, InputWorkspace=x)

        list_input = isinstance(x, list)
        numpy_input = isinstance(x, np.ndarray)
        if numpy_input:
            # If the input is a numpy array reshape into 1D array, saving
            # original shape to reshape output back to it.
            x_list = x.reshape(-1, order="C")
            x_shape = x.shape
        elif list_input:
            # If the input isn't a list, wrap it in one so we can iterate easily
            x_list = x
        else:
            x_list = [x]

        self._set_parameters(params)

        y = x_list[:]
        ws = self._execute_algorithm("CreateWorkspace", DataX=x_list, DataY=y)
        out = self._execute_algorithm("EvaluateFunction", Function=self.fun, InputWorkspace=ws)
        # Create a copy of the calculated spectrum
        output_array = np.array(out.readY(1))
        if numpy_input:
            return output_array.reshape(x_shape, order="C")

        if list_input:
            return output_array
        else:
            return output_array[0]

    def plot(self, **kwargs):  # noqa: C901
        """
        Plot the function

        :param workspace: workspace upon whose x values
                          the function is plotted.
        """
        from mantid import mtd

        try:
            from mantidplot import plot
        except:
            raise RuntimeError("mantidplot must be importable to plot functions.")
        from mantid.simpleapi import CreateWorkspace
        import numpy as np

        isWorkspace = False
        extractSpectrum = False
        workspaceIndex = 0
        haveXValues = False
        haveStartX = False
        haveEndX = False
        nSteps = 20
        plotName = self.name

        def inRange(x):
            return x >= xMin and x <= xMax

        for key in kwargs:
            if key == "workspace":
                isWorkspace = True
                ws = kwargs[key]
                if isinstance(ws, str):
                    ws = mtd[ws]
            if key == "workspaceIndex":
                workspaceIndex = kwargs[key]
                if workspaceIndex > 0:
                    extractSpectrum = True
            if key == "xValues":
                xvals = kwargs[key]
                haveXValues = True
            if key == "startX":
                xMin = kwargs[key]
                haveStartX = True
            if key == "endX":
                xMax = kwargs[key]
                haveEndX = True
            if key == "nSteps":
                nSteps = kwargs[key]
                if nSteps < 1:
                    raise RuntimeError("nSteps must be at least 1")
            if key == "name":
                plotName = kwargs[key]

        if haveStartX and haveEndX:
            if xMin >= xMax:
                raise RuntimeError("startX must be less than EndX")

        if haveXValues:
            spectrumWs = self._execute_algorithm("CreateWorkspace", DataX=xvals, DataY=xvals)
        elif isWorkspace:
            xvals = ws.readX(workspaceIndex)
            if haveStartX and haveEndX:
                xvals = filter(inRange, xvals)
            if extractSpectrum or (haveStartX and haveEndX):
                spectrumWs = self._execute_algorithm("CreateWorkspace", DataX=xvals, DataY=xvals)
            else:
                spectrumWs = ws
        elif haveStartX and haveEndX:
            xvals = np.linspace(start=xMin, stop=xMax, num=nSteps)
            spectrumWs = self._execute_algorithm("CreateWorkspace", DataX=xvals, DataY=xvals)
        else:
            if not haveStartX:
                raise RuntimeError("startX must be defined if no workspace or xValues are defined.")
            if not haveEndX:
                raise RuntimeError("endX must be defined if no workspace or xValues are defined.")
            else:
                raise RuntimeError("insufficient plotting arguments")  # Should not occur.

        outWs = self(spectrumWs)
        vals = outWs.readY(1)
        CreateWorkspace(DataX=xvals, DataY=vals, OutputWorkspace=plotName)
        plot(plotName, 0)

    def tie(self, *args, **kwargs):
        """
        Add ties.

        :param args: one or more dictionaries of ties
        :param kwargs: one or more ties
        """
        for a in args:
            if isinstance(a, dict):
                for key in a:
                    self.fun.tie(key, str(a[key]))

        for key in kwargs:
            self.fun.tie(key, str(kwargs[key]))

    def fix(self, name):
        """
        Fix a parameter.

        :param name: name of parameter to be fixed
        """
        self.fun.fixParameter(name)

    def fixAllParameters(self):
        """
        Fix all parameters.
        """
        self.fun.fixAll()

    def untie(self, name):
        """
        Remove tie from parameter.

        :param name: name of parameter to be untied
        """
        self.fun.removeTie(name)

    def untieAllParameters(self):
        """
        Remove ties from all parameters.
        """
        for i in range(0, self.fun.numParams()):
            self.fun.removeTie(self.getParameterName(i))

    def constrain(self, expressions):
        """
        Add constraints

        :param expressions: string of tie expressions
        """
        self.fun.addConstraints(expressions)

    def unconstrain(self, name):
        """
        Remove constraints from a parameter

        :param name: name of parameter to be unconstrained
        """
        self.fun.removeConstraint(name)

    def free(self, name):
        """
        Free a parameter from tie or constraint

        :param name: name of parameter to be freed
        """
        self.fun.removeTie(name)
        self.fun.removeConstraint(name)

    def getParameterName(self, index):
        """
        Get the name of the parameter of given index

        :param index: index of parameter
        """
        return self.fun.getParamName(index)

    @property
    def function(self):
        """
        Return the underlying IFunction object
        """
        return self.fun

    @property
    def name(self):
        """
        Return the name of the function
        """
        return self.fun.name()

    def _execute_algorithm(self, name, **kwargs):
        alg = AlgorithmManager.createUnmanaged(name)
        alg.setChild(True)
        alg.initialize()
        # Python 3 treats **kwargs as an unordered list meaning it is possible
        # to pass InputWorkspace into EvaluateFunction before Function.
        # As a special case has been made for this. This case can be removed
        # with ordered kwargs change in Python 3.6.
        if name == "EvaluateFunction":
            alg.setProperty("Function", kwargs["Function"])
            del kwargs["Function"]
            alg.setProperty("InputWorkspace", kwargs["InputWorkspace"])
            del kwargs["InputWorkspace"]
        for param in kwargs:
            alg.setProperty(param, kwargs[param])
        alg.setProperty("OutputWorkspace", "none")
        alg.execute()
        return alg.getProperty("OutputWorkspace").value


class CompositeFunctionWrapper(FunctionWrapper):
    """
    Wrapper class for Composite Fitting Function
    """

    def __init__(self, *args, **kwargs):
        """
        Called when creating an instance

        **It should not be called directly**

        :param args:   names of functions in composite function
        :param kwargs: any parameters or attributes that must be passed to the
                       composite function itself.
        """
        self.pureAddition = True
        self.pureMultiplication = False
        self.initByName("CompositeFunction", *args, **kwargs)

    def initByName(self, name, *args, **kwargs):
        """
        intialise composite function of named type.
        E.g. "ProductFunction"
        This function would be protected in c++
        and should not be called directly except
        by :meth:`__init__` functions of this class
        and subclasses.

        :param name:   name of class calling this.
        :param args:   names of functions in composite function
        :param kwargs: any parameters or attributes that must be passed to the
                       composite function itself.
        """
        super().__init__(name=name)
        if len(args) == 1 and not isinstance(args[0], FunctionWrapper):
            # We have a composite function to wrap
            self.fun = args[0]
        else:
            self.fun = FunctionFactory.createCompositeFunction(name)

            # Add the functions, checking for Composite & Product functions
            for a in args:
                if not isinstance(a, int):
                    if isinstance(a, CompositeFunctionWrapper):
                        if self.pureAddition:
                            self.pureAddition = a.pureAddition
                        if self.pureMultiplication:
                            self.pureMultiplication = a.pureMultiplication
                    functionToAdd = FunctionFactory.createInitialized(a.fun.__str__())
                    self.fun.add(functionToAdd)
        self.init_paramgeters_and_attributes(**kwargs)

    def getParameter(self, name):
        """
        get value of parameter of specified name

        :param name: name of parameter
        """
        return self.fun.getParameterValue(name)

    def getCompositeParameterName(self, name, index):
        """
        get composite parameter name of parameter of
        given name of member function of given index
        """
        return "f" + str(index) + "." + name

    def getIndexOfFunction(self, name):
        """
        get index of function specified by name,
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
                if count == occurrence:
                    return index
                else:
                    count += 1
            index += 1

        raise RuntimeError("Specified function not found.")

    def f(self, name):
        """
        get function specified by name,
        such as "LinearBackground" for the only
        LinearBackground function or
        "Gaussian1" for the second Gaussian function.

        :param name: name specifying the function
        """
        index = self.getIndexOfFunction(name)
        return self[index]

    def __getitem__(self, nameorindex):
        """
        get function of specified index or parameter of specified name
        called for array-like access on RHS.

        **It should not be called directly.**

        :param name: name or index in the []
        """

        comp = self.fun
        if (isinstance(nameorindex, str) and not comp.hasParameter(nameorindex)) or (
            isinstance(nameorindex, int) and nameorindex >= comp.nParams()
        ):
            raise AttributeError("Parameter %s not found" % nameorindex)
        item = comp[nameorindex]
        if isinstance(item, float):
            return item
        elif item.name() == "CompositeFunction":
            return CompositeFunctionWrapper(item)
        elif item.name() == "ProductFunction":
            return ProductFunctionWrapper(item)
        elif item.name() == "Convolution":
            return ConvolutionWrapper(item)
        else:
            return FunctionWrapper(item)

    def __setitem__(self, name, newValue):
        """
        Called from array-like access on LHS

        **It should not be called directly.**

        :param name: name or index in the []
        :param newValue: new value for item
        """
        comp = self.fun
        if isinstance(newValue, FunctionWrapper):
            comp[name] = newValue.fun
        else:
            comp[name] = newValue

    def __iadd__(self, other):
        """
        Implement += operator.

        **It should not be called directly.**

        :param other: object to add
        """
        self.fun.add(other.fun)
        return self

    def __delitem__(self, index):
        """
        Delete item of given index from composite function.

        **It should not be called directly.**

        :param index: index of item
        """
        self.fun.__delitem__(index)

    def __len__(self):
        """
        Return number of items in composite function.
        Implement len() function.

        **It should not be called directly.**
        """

        composite = self.fun
        return composite.__len__()

    def tieAll(self, name):
        """
        For each member function, tie the parameter of the given name
        to the parameter of that name in the first member function.
        The named parameter must occur in all the member functions.

        :param name: name of parameter
        """
        expr = self.getCompositeParameterName(name, 0)
        self.tie({self.getCompositeParameterName(name, i): expr for i in range(1, len(self))})

    def fixAll(self, name):
        """
        Fix all parameters with the given local name.
        Every member function must have a parameter of this name.

        :param name: name of parameter
        """
        for f in self:
            f.fix(name)

    def constrainAll(self, expressions):
        """
        Constrain all parameters according local names in expressions.

        :param expressions: string of expressions
        """
        for i in range(0, len(self)):
            if isinstance(self[i], CompositeFunctionWrapper):
                self[i].constrainAll(expressions)
            else:
                try:
                    self[i].constrain(expressions)
                except:
                    pass

    def unconstrainAll(self, name):
        """
        Unconstrain all parameters of given local name.

        :param name: local name of parameter
        """
        for i in range(0, len(self)):
            if isinstance(self[i], CompositeFunctionWrapper):
                self[i].unconstrainAll(name)
            else:
                try:
                    self[i].unconstrain(name)
                except:
                    pass

    def untieAll(self, name):
        """
        Untie all parameters with the given local name.
        Every member function must have a parameter of this name.

        :param name: local name of parameter
        """
        for i in range(0, len(self)):
            self.untie(self.getCompositeParameterName(name, i))

    def flatten(self):
        """
        Return composite function, equal to self, but with
        every composite function within replaced by
        its list of functions, so having a pure list of functions.
        This makes it possible to index and iterate all the functions
        and use :meth:`tieAll` and :meth:`untieAll`.
        Not to be used with a mixture of product and sum
        composite functions, because the arithmetic
        may no longer be correct.
        The return value is not independent of self.
        """
        # If there are no composite functions, do nothing
        needToFlatten = False
        for i in range(0, len(self)):
            if not needToFlatten and isinstance(self[i], CompositeFunctionWrapper):
                needToFlatten = True

        if not needToFlatten:
            return self

        # Now we know there is a composite function.
        if isinstance(self, ProductFunctionWrapper):
            flatSelf = ProductFunctionWrapper()
        else:
            flatSelf = CompositeFunctionWrapper()

        for i in range(0, len(self)):
            if isinstance(self[i], CompositeFunctionWrapper):
                currentFunction = self[i].flatten()
                for j in range(0, len(currentFunction)):
                    flatSelf.fun.add(currentFunction[j].fun)
            else:
                flatSelf.fun.add(self[i].fun)

        return flatSelf


class ProductFunctionWrapper(CompositeFunctionWrapper):
    """
    Wrapper class for Product Fitting Function
    """

    def __init__(self, *args, **kwargs):
        """
        Called when creating an instance

        **It should not be called directly.**

        :param args:   names of functions in composite function
        :param kwargs: any parameters or attributes that must be passed to the
                       composite function itself.
        """
        self.pureAddition = False
        self.pureMultiplication = True
        self.initByName("ProductFunction", *args, **kwargs)


class ConvolutionWrapper(CompositeFunctionWrapper):
    """
    Wrapper class for Convolution Fitting Function
    """

    def __init__(self, *args, **kwargs):
        """
        Called when creating an instance

        **It should not be called directly.**

        :param args:   names of functions in composite function
        :param kwargs: any parameters or attributes that must be passed to the
                       composite function itself.
        """
        self.pureAddition = False
        self.pureMultiplication = False
        self.initByName("Convolution", *args, **kwargs)


class MultiDomainFunctionWrapper(CompositeFunctionWrapper):
    """
    Wrapper class for Multidomain Fitting Function
    """

    def __init__(self, *args, **kwargs):
        """
        Called when creating an instance

        **It should not be called directly**

        :param args:   names of functions in composite function
        :param kwargs: any parameters or attributes that must be passed to the
                       composite function itself.
        """
        # Assume it's not safe to flatten
        self.pureAddition = False
        self.pureMultiplication = False

        global_parameters = []
        if "Global" in kwargs:
            global_parameters = kwargs["Global"]
            del kwargs["Global"]

        # Create and populate with copied functions
        self.initByName("MultiDomainFunction", *args, **kwargs)

        # Tie the global parameters
        for name in global_parameters:
            self.tieAll(name)

        # Set domain indices: 1 to 1
        for i in range(0, len(self)):
            self.fun.setDomainIndex(i, i)

    @property
    def nDomains(self):
        """
        Return number of domains
        """
        return self.fun.nDomains()


_name_to_constructor_map = {
    "CompositeFunction": CompositeFunctionWrapper,
    "ProductFunction": ProductFunctionWrapper,
    "Convolution": ConvolutionWrapper,
    "MultiDomainFunction": MultiDomainFunctionWrapper,
}

# Some functions need to be excluded from wrapping, eg
# if there is an algorithm with the same name.
_do_not_wrap = ["VesuvioResolution"]


def _create_wrapper_function(name):
    """
    Create fake functions for the given name

    **It should not be called directly**

    :param name: name of fake function
    """

    # ------------------------------------------------------------------------------------------------
    def wrapper_function(*args, **kwargs):
        return FunctionWrapper.wrap(name, *args, **kwargs)

    # ------------------------------------------------------------------------------------------------
    wrapper_function.__name__ = name
    return wrapper_function


def _wrappers():
    for name in FunctionFactory.getFunctionNames():
        # Wrap all registered functions which are not in the black list
        if name not in _do_not_wrap:
            yield name, _create_wrapper_function(name)


_ExportedIFunction1D = IFunction1D


class _IFunction1DWrapperCreator(_ExportedIFunction1D):
    """Overriden IFunction1D class that allows creation of
    FunctionWrappers for newly defined fit functions.

    User defined function inherit from this class.
    When the user calls the constructor to create an instance
    of the new function it returns a FunctionWrapper that
    wraps the actual fit function.

    Class field _used_by_factory is set by FunctionFactory
    to indicate that an unwrapped instance of the fit function
    must be returned by the constructor.
    """

    _used_by_factory = False

    def __new__(cls, **kwargs):
        if cls._used_by_factory:
            return _ExportedIFunction1D.__new__(cls)
        return FunctionWrapper(cls.__name__, **kwargs)

    @classmethod
    def _factory_use(cls):
        cls._used_by_factory = True

    @classmethod
    def _factory_free(cls):
        cls._used_by_factory = False


IFunction1D = _IFunction1DWrapperCreator
