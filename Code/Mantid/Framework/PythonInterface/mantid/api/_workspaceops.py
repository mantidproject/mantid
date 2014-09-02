"""
    This module adds functions to  the Workspace classes
    so that Python operators, i.e +-*/,  can be used on them

    It is intended for internal use.
"""
from mantid.kernel.funcreturns import lhs_info
import _api

import inspect as _inspect

#------------------------------------------------------------------------------
# Binary Ops
#------------------------------------------------------------------------------
def attach_binary_operators_to_workspace():
    """
        Attaches the common binary operators
        to the Workspace class
    """
    def add_operator_func(attr, algorithm, inplace, reverse):
        # Wrapper for the function call
        def op_wrapper(self, other):
            # Get the result variable to know what to call the output
            result_info = lhs_info()
            # Pass off to helper
            return _do_binary_operation(algorithm, self, other, result_info,
                                 inplace, reverse)
        op_wrapper.__name__ = attr
        setattr(_api.Workspace, attr, op_wrapper)
    # Binary operations that workspaces are aware of
    operations = {
        "Plus":("__add__", "__radd__","__iadd__"),
        "Minus":("__sub__", "__rsub__","__isub__"),
        "Multiply":("__mul__", "__rmul__","__imul__"),
        "Divide":("__div__", "__div__","__idiv__"),
        "LessThan":"__lt__",
        "GreaterThan":"__gt__",
        "Or":"__or__",
        "And":"__and__",
        "Xor":"__xor__"
    }
    # Loop through and add each one in turn
    for alg, attributes in operations.iteritems():
        if type(attributes) == str: attributes = [attributes]
        for attr in attributes:
            add_operator_func(attr, alg, attr.startswith('__i'), attr.startswith('__r'))

# Prefix for temporary objects within workspace operations
_workspace_op_prefix = '__python_op_tmp'
# A list of temporary workspaces created by algebraic operations
_workspace_op_tmps = []

def _do_binary_operation(op, self, rhs, lhs_vars, inplace, reverse):
    """
        Perform the given binary operation

        :param op: A string containing the Mantid algorithm name
        :param self: The object that was the self argument when object.__op__(other) was called
        :param rhs: The object that was the other argument when object.__op__(other) was called
        :param lhs_vars: A tuple containing details of the lhs of the assignment, i.e a = b + c, lhs_vars = (1, 'a')
        :param inplace: True if the operation should be performed inplace
        :param reverse: True if the reverse operator was called, i.e. 3 + a calls __radd__

    """
    global _workspace_op_tmps
    #
    if lhs_vars[0] > 0:
        # Assume the first and clear the temporaries as this
        # must be the final assignment
        if inplace:
            output_name = self.name()
        else:
            output_name = lhs_vars[1][0]
        clear_tmps = True
    else:
        # Give it a temporary name and keep track of it
        clear_tmps = False
        output_name = _workspace_op_prefix + str(len(_workspace_op_tmps))

    # Do the operation
    resultws = _api.performBinaryOp(self,rhs, op, output_name, inplace, reverse)

    # Do we need to clean up
    if clear_tmps:
        ads = _api.AnalysisDataServiceImpl.Instance()
        for name in _workspace_op_tmps:
            if name in ads and output_name != name:
                del ads[name]
        _workspace_op_tmps = []
    else:
        if type(resultws) == _api.WorkspaceGroup:
            # Ensure the members are removed aswell
            members = resultws.getNames()
            for member in members:
                _workspace_op_tmps.append(member)
        else:
            _workspace_op_tmps.append(output_name)

    return resultws # For self-assignment this will be set to the same workspace

#------------------------------------------------------------------------------
# Unary Ops
#------------------------------------------------------------------------------
def attach_unary_operators_to_workspace():
    """
        Attaches the common unary operators
        to the Workspace class
    """
    def add_operator_func(attr, algorithm):
        # Wrapper for the function call
        def op_wrapper(self):
            # Get the result variable to know what to call the output
            result_info = lhs_info()
            # Pass off to helper
            return _do_unary_operation(algorithm, self, result_info)
        op_wrapper.__name__ = attr
        setattr(_api.Workspace, attr, op_wrapper)
    # Binary operations that workspaces are aware of
    operations = {
        'NotMD':'__invert__'
    }
    # Loop through and add each one in turn
    for alg, attributes in operations.iteritems():
        if type(attributes) == str: attributes = [attributes]
        for attr in attributes:
            add_operator_func(attr, alg)


def _do_unary_operation(op, self, lhs_vars):
    """
    Perform the unary operation

    :param op: name of the algorithm to run
    :param self: The object that this operation was called on
    :param lhs_vars: is expected to be a tuple containing the number of lhs variables and
            their names as the first and second element respectively
    """
    global _workspace_op_tmps
    import mantid.simpleapi as simpleapi

    if lhs_vars[0] > 0:
        # Assume the first and clear the temporaries as this
        # must be the final assignment
        output_name = lhs_vars[1][0]
        clear_tmps = True
    else:
        # Give it a temporary name and keep track of it
        clear_tmps = False
        output_name = _workspace_op_prefix + str(len(_workspace_op_tmps))
        _workspace_op_tmps.append(output_name)

    # Do the operation
    ads = _api.AnalysisDataServiceImpl.Instance()

    alg = simpleapi._create_algorithm_object(op) # gets the child status correct for PythonAlgorithms
    alg.setPropertyValue("InputWorkspace", self.name())
    alg.setPropertyValue("OutputWorkspace", output_name)
    alg.execute()
    resultws = ads[output_name]

    if clear_tmps:
        for name in _workspace_op_tmps:
            if name in ads and output_name != name:
                ads.remove(name)
        _workspace_op_tmps = []

    return resultws

#------------------------------------------------------------------------------
# TableWorkspace Operations
#------------------------------------------------------------------------------
def attach_tableworkspaceiterator():
    """Attaches the iterator code to a table workspace."""
    def __iter_method(self):
        class ITableWorkspaceIter:
            def __init__(self, wksp):
                self.__wksp = wksp
                self.__pos = 0
                self.__max = wksp.rowCount()
            def next(self):
                if self.__pos + 1 > self.__max:
                    raise StopIteration
                self.__pos += 1
                return self.__wksp.row(self.__pos-1)
        return ITableWorkspaceIter(self)

    setattr(_api.ITableWorkspace, "__iter__", __iter_method)

#------------------------------------------------------------------------------
# Algorithms as workspace methods
#------------------------------------------------------------------------------
def attach_func_as_method(name, func_obj, self_param_name, workspace_types=None):
    """
        Adds a method to the given type that calls an algorithm
        using the calling object as the input workspace

        :param name: The name of the new method as it should appear on the type
        :param func_obj: A free function object that defines the implementation of the call
        :param self_param_name: The name of the parameter in the free function that the method's self maps to
        :param workspace_types: A list of string names of a workspace types. If None, then it is attached
                              to the general Workspace type. Default=None
    """
    def _method_impl(self, *args, **kwargs):
        # Map the calling object to the requested parameter
        kwargs[self_param_name] = self
        # Define the frame containing the final variable assignment
        # used to figure out the workspace name
        kwargs["__LHS_FRAME_OBJECT__"] = _inspect.currentframe().f_back
        # Call main function
        return func_obj(*args, **kwargs)
    #------------------------------------------------------------------
    # Add correct meta-properties for the method
    _method_impl.__name__ = func_obj.__name__
    _method_impl.__doc__ = func_obj.__doc__
    f = _method_impl.func_code
    signature = ['self']
    signature.extend(func_obj.func_code.co_varnames)
    c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, f.co_consts, f.co_names,
                  tuple(signature), f.co_filename, f.co_name, f.co_firstlineno, f.co_lnotab, f.co_freevars)
    # Replace the code object of the wrapper function
    _method_impl.func_code = c

    if workspace_types or len(workspace_types) > 0:
        for typename in workspace_types:
            cls = getattr(_api, typename)
            setattr(cls, name, _method_impl)
    else:
        setattr(_api.Workspace, name, _method_impl)
