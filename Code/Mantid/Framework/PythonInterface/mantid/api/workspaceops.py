"""
    This module contains functions to add attributes to the
    the Workspace classes so that Python operators, i.e +-*/,
    can be used on them
    
    It is intended for internal use.
"""
from mantid.api import Workspace, AnalysisDataService
from mantid.api import performBinaryOp as _perform_binary_op
from mantid.kernel.funcreturns import lhs_info

_ads = AnalysisDataService.Instance()

def attach_binary_operators_to_workspace():
    """
        Attaches the common binary operators
        to the Workspace class
    """
    operations = {
        "Plus":("__add__", "__radd__","_iadd__"),
        "Minus":("__sub__", "__rsub__","_isub__"),
        "Multiply":("__mul__", "__rmul__","_imul__"),
        "Divide":("__div__", "__div__","_idiv__"),
        "LessThan":("__lt__"),
        "GreaterThan":("__gt__"),
        "Or":("__or__"),
        "And":("__and__"),
        "Xor":("__xor__")
    }
    for alg, attributes in operations.iteritems():
        for attr in attributes:
            add_operator_func(attr, alg, attr.startswith('__i'), attr.startswith('__r'))

def add_operator_func(attr, algorithm, inplace, reverse):
    # Wrapper for the function call
    def op_wrapper(self, other):
        # Get the result variable to know what to call the output
        result_info = lhs_info()
        # Pass off to helper
        return _do_operation(algorithm, self, other, result_info, 
                             inplace, reverse)
    op_wrapper.__name__ = attr
    setattr(Workspace, attr, op_wrapper)

# Prefix for temporary objects within workspace binary operations
_binary_op_prefix = '__binary_tmp'
# A list of temporary workspaces created by algebraic operations
_binary_op_tmps = []

def _do_operation(op, self, rhs, lhs_vars, inplace, reverse):
    """
        Perform the given binary operation

        @param op A string containing the Mantid algorithm name
        @param self The object that was the self argument when object.__op__(other) was called
        @param rhs The object that was the other argument when object.__op__(other) was called
        @param lhs_vars A tuple containing details of the lhs of the assignment, i.e a = b + c, lhs_vars = (1, 'a')
        @param inplace True if the operation should be performed inplace
        @param reverse True if the reverse operator was called, i.e. 3 + a calls __radd__
        
    """
    global _binary_op_tmps
    print 'Performing',op,'with',self,rhs,'to give',lhs_vars[1]
    if reverse: print 'Operation will be reversed'
    if inplace: print 'Operation is inplace'
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
        output_name = _binary_op_prefix + str(len(_binary_op_tmps))
        _binary_op_tmps.append(output_name)

    # Do the operation
    resultws = _perform_binary_op(self,rhs, op, output_name, inplace, reverse)

    if clear_tmps:
        for name in _binary_op_tmps:
            if name in _ads and output_name != name:
                del _ads[name]
        _binary_op_tmps = []
        
    if inplace:
        return self
    else:
        return resultws
