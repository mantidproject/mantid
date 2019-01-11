# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Defines functions that can be used to inspect the properties of a
    function call. For example

        lhs_info() can be used to get retrieve the names and number of
                   arguments that are being assigned to a function
                   return
"""
from __future__ import (absolute_import, division,
                        print_function)
import opcode
import inspect
import sys
import dis
from six import PY3


def replace_signature(func, varnames):
    """
    Replace the signature of the given function object with that given by
    varnames
    :param func: Function whose signature is to be replaced
    :param varnames: A tuple of names of arguments and local variables
    """
    # Code object is different in Python 3
    if hasattr(func, 'func_code'):
        # Version 2
        code_attr = 'func_code'
        f = func.func_code
        c = f.__new__(f.__class__, f.co_argcount, f.co_nlocals, f.co_stacksize,
                      f.co_flags, f.co_code, f.co_consts, f.co_names,
                      varnames, f.co_filename, f.co_name, f.co_firstlineno,
                      f.co_lnotab, f.co_freevars)
    else:
        code_attr = '__code__'
        f = func.__code__
        c = f.__new__(f.__class__, f.co_argcount, f.co_kwonlyargcount,
                      f.co_nlocals, f.co_stacksize, f.co_flags, f.co_code, 
                      f.co_consts, f.co_names, varnames, 
                      f.co_filename, f.co_name, f.co_firstlineno,
                      f.co_lnotab, f.co_freevars)
    #endif
    setattr(func, code_attr, c)


def customise_func(func, name, signature, docstring):
    """
    Takes the definition of the algorithm function and replaces
    the attributes of the instance to make it look like a handwritten
    function definition
    :param func: A function object holding the definition
    :param name: The name of the algorithm
    :param signature: A new signature for the function. Expecting a 2-tuple
                      of arguments and local variables. See _replace_signature
    :param docstring: A string containing the function documentation
    """
    func.__name__ = str(name)
    func.__doc__ = docstring
    replace_signature(func, signature)
    return func

#-------------------------------------------------------------------------------
def decompile(code_object):
    """
    Taken from
    http://thermalnoise.wordpress.com/2007/12/30/exploring-python-bytecode/

    Extracts dissasembly information from the byte code and stores it in
    a list for further use.

    Call signature(s):
        instructions=decompile(f.f_code)

    Required      arguments:
    =========     =====================================================================
    code_object   A bytecode object extracted with inspect.currentframe()
                  or any other mechanism that returns byte code.

    Optional keyword arguments: NONE

    Outputs:
    =========     =====================================================================
    instructions  a list of offsets, op_codes, names, arguments,
                  argument_value which can be deconstructed to find out various things
                  about a function call.

    Example:
    # Two frames back so that we get the callers' caller
    f = inspect.currentframe().f_back.f_back
    i = f.f_lasti  # index of the last attempted instruction in byte code
    ins = decompile(f.f_code)
    """
    instructions = []

    if PY3:
        for ins in dis.get_instructions(code_object):
            instructions.append( (ins.offset, ins.opcode, ins.opname, ins.arg, ins.argval) )
    else:
        code = code_object.co_code
        variables = code_object.co_cellvars + code_object.co_freevars
        n = len(code)
        i = 0
        e = 0
        while i < n:
            i_offset = i
            i_opcode = ord(code[i])
            i = i + 1
            if i_opcode >= opcode.HAVE_ARGUMENT:
                i_argument = ord(code[i]) + (ord(code[i+1]) << (4*2)) + e
                i = i + 2
                if i_opcode == opcode.EXTENDED_ARG:
                    e = i_argument << 16
                else:
                    e = 0
                if i_opcode in opcode.hasconst:
                    i_arg_value = repr(code_object.co_consts[i_argument])
                elif i_opcode in opcode.hasname:
                    i_arg_value = code_object.co_names[i_argument]
                elif i_opcode in opcode.hasjrel:
                    i_arg_value = repr(i + i_argument)
                elif i_opcode in opcode.haslocal:
                    i_arg_value = code_object.co_varnames[i_argument]
                elif i_opcode in opcode.hascompare:
                    i_arg_value = opcode.cmp_op[i_argument]
                elif i_opcode in opcode.hasfree:
                    i_arg_value = variables[i_argument]
                else:
                    i_arg_value = i_argument
            else:
                i_argument = None
                i_arg_value = None
            instructions.append( (i_offset, i_opcode, opcode.opname[i_opcode], i_argument, i_arg_value) )
    return instructions

#-------------------------------------------------------------------------------

# A must list all of the operators that behave like a function calls in byte-code
# This is for the lhs functionality
__operator_names = set(['CALL_FUNCTION', 'CALL_FUNCTION_VAR', 'CALL_FUNCTION_KW',
                        'CALL_FUNCTION_VAR_KW','UNARY_POSITIVE',
                        'UNARY_NEGATIVE','UNARY_NOT', 'UNARY_CONVERT','UNARY_INVERT',
                        'GET_ITER', 'BINARY_POWER', 'BINARY_MULTIPLY','BINARY_DIVIDE',
                        'BINARY_FLOOR_DIVIDE', 'BINARY_TRUE_DIVIDE', 'BINARY_MODULO',
                        'BINARY_ADD','BINARY_SUBTRACT', 'BINARY_SUBSCR','BINARY_LSHIFT',
                        'BINARY_RSHIFT','BINARY_AND', 'BINARY_XOR','BINARY_OR',
                        'INPLACE_POWER', 'INPLACE_MULTIPLY', 'INPLACE_DIVIDE',
                        'INPLACE_TRUE_DIVIDE','INPLACE_FLOOR_DIVIDE',
                        'INPLACE_MODULO', 'INPLACE_ADD', 'INPLACE_SUBTRACT',
                        'INPLACE_LSHIFT','INPLACE_RSHIFT','INPLACE_AND', 'INPLACE_XOR',
                        'INPLACE_OR', 'COMPARE_OP',
                        'CALL_FUNCTION_EX', 'LOAD_METHOD', 'CALL_METHOD'])
#--------------------------------------------------------------------------------------

def process_frame(frame):
    """Returns the number of arguments on the left of assignment along
    with the names of the variables for the given frame.

    Call signature(s)::

    Required arguments:
    ===========================   ==========
    frame                         The code frame to analyse

    Outputs:
    =========
    Returns the a tuple with the number of arguments and their names
    """
    # Index of the last attempted instruction in byte code
    last_i = frame.f_lasti
    ins_stack = decompile(frame.f_code)

    call_function_locs = {}
    start_index = 0
    start_offset = 0

    for index, instruction in enumerate(ins_stack):
        (offset, op, name, argument, argvalue) = instruction
        if name in __operator_names:
            call_function_locs[start_offset] = (start_index, index)
            start_index = index
            start_offset = offset

    (offset, op, name, argument, argvalue) = ins_stack[-1]
    # Append the index of the last entry to form the last boundary
    call_function_locs[start_offset] = (start_index, len(ins_stack)-1)

    # last_i should be the offset of a call_function_locs instruction.
    # We use this to bracket the bit which we are interested in.
    # Bug:
    # Some types of call, eg
    #    def foo(callableObj, *args, **kwargs):
    #        x = callableObj(*args, **kwargs)
    #
    #     foo(FuncUsingLHS, 'Args')
    #
    # have the incorrect index at last_i due to the call being passed through
    # an intermediate reference. Currently this method does not provide the
    # correct answer and throws a KeyError. Ticket #4186
    output_var_names = []
    max_returns = []
    last_func_offset = call_function_locs[last_i][0]
    (offset, op, name, argument, argvalue) = ins_stack[last_func_offset + 1]
    if name == 'POP_TOP':  # no return values
        pass
    if name == 'STORE_FAST' or name == 'STORE_NAME': # one return value
        output_var_names.append(argvalue)
    if name == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign
        for index in range(argvalue):
            (offset_, op_, name_, argument_, argvalue_) = ins_stack[last_func_offset + 2 +index]
            output_var_names.append(argvalue_)
    max_returns = len(output_var_names)
    if name == 'DUP_TOP': # Many Return Values, Many equal signs
        # The output here should be a multi-dim list which mimics the variable unpacking sequence.
        # For instance a,b=c,d=f() => [ ['a','b'] , ['c','d'] ]
        #              a,b=c=d=f() => [ ['a','b'] , 'c','d' ]  So on and so forth.

        # put this in a loop and stack the results in an array.
        count = 0
        max_returns = 0 # Must count the max_returns ourselves in this case
        while count < len(ins_stack[call_function_locs[i][0]:call_function_locs[i][1]]):
            (offset_, op_, name_, argument_, argvalue_) = ins[call_function_locs[i][0]+count]
            if name_ == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign
                hold = []
                if argvalue_ > max_returns:
                    max_returns = argvalue_
                for index in range(argvalue_):
                    (_offset_, _op_, _name_, _argument_, _argvalue_) = ins[call_function_locs[i][0] + count+1+index]
                    hold.append(_argvalue_)
                count = count + argvalue_
                output_var_names.append(hold)
            # Need to now skip the entries we just appended with the for loop.
            if name_ == 'STORE_FAST' or name_ == 'STORE_NAME': # One Return Value
                if 1 > max_returns:
                    max_returns = 1
                output_var_names.append(argvalue_)
            count = count + 1

    return (max_returns, tuple(output_var_names))

#-------------------------------------------------------------------------------

def lhs_info(output_type='both', frame=None):
    """Returns the number of arguments on the left of assignment along
    with the names of the variables.

    Acknowledgements:
       Thanks to Tim Charlton and Jon Taylor of the ISIS facility for
       figuring this out.

    Call signature(s)::

    Required arguments: NONE

    Optional keyword arguments    Meaning:
    ===========================   ==========
    output_type                   A string enumerating the type of output, one of
                                    output_type = 'nreturns' : The number of return values
                                                      expected from the call
                                    output_type = 'names' : Just return a list of
                                                      variable names
                                    output_type = 'both' : A tuple containing both of
                                                      the above
    frame                         A frame object that points to the frame containing a variable assignment.
                                  Default = inspect.currentframe().f_back.f_back

    Outputs:
    =========
    Depends on the value of the argument. See above.

    """
    if not frame:
        try:
            # Two frames back so that we get the callers' caller, i.e. this should only
            # be called from within a function
            frame = inspect.currentframe().f_back.f_back
        except AttributeError:
            raise RuntimeError("lhs_info cannot be used on the command line, only within a function")

    # Process the frame noting the advice here:
    # http://docs.python.org/library/inspect.html#the-interpreter-stack
    try:
        ret_vals = process_frame(frame)
    finally:
        del frame

    if output_type == 'nreturns':
        ret_vals = ret_vals[0]
    elif output_type == 'names':
        ret_vals = ret_vals[1]
    else:
        pass

    return ret_vals

#-------------------------------------------------------------------------------
