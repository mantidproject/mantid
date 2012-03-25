"""
    This module adds functions to  the Workspace classe
    so that Python operators, i.e +-*/,  can be used on them
    
    It is intended for internal use.
"""
from mantid.api import Workspace, AnalysisDataService, FrameworkManager, ITableWorkspace
from mantid.api import performBinaryOp as _performBinaryOp
from mantid.kernel.funcreturns import lhs_info

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
        setattr(Workspace, attr, op_wrapper)
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

        @param op A string containing the Mantid algorithm name
        @param self The object that was the self argument when object.__op__(other) was called
        @param rhs The object that was the other argument when object.__op__(other) was called
        @param lhs_vars A tuple containing details of the lhs of the assignment, i.e a = b + c, lhs_vars = (1, 'a')
        @param inplace True if the operation should be performed inplace
        @param reverse True if the reverse operator was called, i.e. 3 + a calls __radd__
        
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
        _workspace_op_tmps.append(output_name)

    # Do the operation
    resultws = _performBinaryOp(self,rhs, op, output_name, inplace, reverse)

    if clear_tmps:
        for name in _workspace_op_tmps:
            if name in AnalysisDataService and output_name != name:
                del AnalysisDataService[name]
        _workspace_op_tmps = []
        
    if inplace:
        return self
    else:
        return resultws

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
        setattr(Workspace, attr, op_wrapper)
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

    @param op :: name of the algorithm to run
    @param self :: The object that this operation was called on
    @param lhs_vars :: is expected to be a tuple containing the number of lhs variables and
            their names as the first and second element respectively
    """
    global _workspace_op_tmps

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
    alg = FrameworkManager.createAlgorithm(op)
    alg.setPropertyValue("InputWorkspace", self.name())
    alg.setPropertyValue("OutputWorkspace", output_name)
    alg.execute()
    resultws = AnalysisDataService[output_name]

    if clear_tmps:
        for name in _workspace_op_tmps:
            if name in AnalysisDataService and output_name != name:
                AnalysisDataService.remove(name)
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

    setattr(ITableWorkspace, "__iter__", __iter_method)

#------------------------------------------------------------------------------
# Attach the operators
#------------------------------------------------------------------------------
attach_binary_operators_to_workspace()
attach_unary_operators_to_workspace()
attach_tableworkspaceiterator()
