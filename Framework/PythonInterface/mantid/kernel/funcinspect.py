# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Defines functions that can be used to inspect the properties of a
function call. For example

    lhs_info() can be used to get retrieve the names and number of
               arguments that are being assigned to a function
               return
"""

import inspect
import dis


def customise_func(func, name, signature, docstring):
    """
    Takes the definition of the algorithm function and replaces
    the attributes of the instance to make it look like a handwritten
    function definition
    :param func: A function object holding the definition
    :param name: The name of the algorithm
    :param signature: A new signature for the function
    :param docstring: A string containing the function documentation
    """
    func.__name__ = str(name)
    func.__doc__ = docstring
    func.__signature__ = signature
    return func


# -------------------------------------------------------------------------------


class LazyFunctionSignature(inspect.Signature):
    """
    Allows for lazy access to the signature of a function, only generating it when it is requested
    to reduce the time spent initialising algorithms.
    """

    __slots__ = ("_alg_name", "__sig", "_include_self")

    def __init__(self, *args, **kwargs):
        if "alg_name" not in kwargs:
            super().__init__(*args, **kwargs)
            self.__sig = self
        else:
            self._alg_name = kwargs.pop("alg_name")
            self.__sig = None

        if "include_self" in kwargs:
            self._include_self = kwargs.pop("include_self")
        else:
            self._include_self = True

    @property
    def _signature(self):
        if self.__sig is None:
            self.__sig = self._create_signature(self._alg_name)

        return self.__sig

    def __getattr__(self, item):
        # Called for each attribute access.
        if item in LazyFunctionSignature.__slots__:
            return getattr(self, item)
        else:
            return getattr(self._signature, item)

    def _create_signature(self, alg_name):
        from inspect import Signature

        return Signature(self._create_parameters(alg_name))

    def _create_parameters(self, alg_name):
        from mantid.api import AlgorithmManager

        alg_object = AlgorithmManager.Instance().createUnmanaged(alg_name)
        alg_object.initialize()
        from inspect import Parameter

        pos_or_keyword = Parameter.POSITIONAL_OR_KEYWORD
        parameters = []
        for name in alg_object.mandatoryProperties():
            prop = alg_object.getProperty(name)
            # Mandatory parameters are those for which the default value is not valid
            if isinstance(prop.isValid, str):
                valid_str = prop.isValid
            else:
                valid_str = prop.isValid()
            if len(valid_str) > 0:
                parameters.append(Parameter(name, pos_or_keyword))
            else:
                # None is not quite accurate here, but we are reproducing the
                # behavior found in the C++ code for SimpleAPI.
                parameters.append(Parameter(name, pos_or_keyword, default=None))
        if self._include_self:
            # Add a self parameter since these are called from a class.
            parameters.insert(0, Parameter("self", Parameter.POSITIONAL_ONLY))
        return parameters


class LazyMethodSignature(LazyFunctionSignature):
    """
    Alternate LazyFunctionSignature intended for use in workspace methods. Replaces the input workspace
    parameter with self.
    """

    def _create_parameters(self, alg_name):
        from inspect import Parameter

        parameters = super()._create_parameters(alg_name)
        try:
            parameters.pop(0)
        except IndexError:
            pass
        parameters.insert(0, Parameter("self", Parameter.POSITIONAL_ONLY))
        return parameters


# -------------------------------------------------------------------------------


def decompile(code_object, show_caches=False):
    """
    Taken from
    http://thermalnoise.wordpress.com/2007/12/30/exploring-python-bytecode/

    Extracts disassembly information from the byte code and stores it in
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
    for ins in dis.get_instructions(code_object, show_caches=show_caches):
        instructions.append((ins.offset, ins.opcode, ins.opname, ins.arg, ins.argval))
    return instructions


# We must list all of the operators that behave like a function calls in byte-code
# This is for the lhs functionality
OPERATOR_NAMES = {
    "CALL",
    "CALL_FUNCTION_VAR",
    "CALL_FUNCTION_VAR_KW",
    "UNARY_POSITIVE",
    "UNARY_NEGATIVE",
    "UNARY_NOT",
    "UNARY_CONVERT",
    "UNARY_INVERT",
    "GET_ITER",
    "BINARY_OP",
    "COMPARE_OP",
    "CALL_FUNCTION_EX",
    "LOAD_METHOD",
    "DICT_MERGE",
    "DICT_UPDATE",
    "LIST_EXTEND",
    "SET_UPDATE",
    "BUILD_CONST_KEY_MAP",
}


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
    ins_stack_with_caches = decompile(frame.f_code, show_caches=True)
    ins_stack = decompile(frame.f_code)

    call_function_locs = {}
    start_index = 0
    start_offset = 0

    for index, instruction in enumerate(ins_stack):
        offset, _, name, _, _ = instruction
        if name in OPERATOR_NAMES:
            call_function_locs[start_offset] = (start_index, index)
            start_index = index
            start_offset = offset

    # Append the index of the last entry to form the last boundary
    call_function_locs[start_offset] = (start_index, len(ins_stack) - 1)

    current_instruction = [name for offset, _, name, _, _ in ins_stack_with_caches if offset == last_i][0]
    if current_instruction == "CACHE":
        # Since 3.11, some bytcode instructions have several CACHE instructions after them.
        # last_i will be pointing at the last of those CACHE instructions after the call function.
        # We want it pointing at the call fuction, as assign it to the largets offset in call_function_locs
        # which is < last_i
        # https://docs.python.org/3/whatsnew/3.11.html#cpython-bytecode-changes
        last_i = [offset for offset in call_function_locs.keys() if offset < last_i][-1]
    elif current_instruction == "PRECALL":
        # Handle the PRECALL instruction added in 3.11. Sometimes (can't find a pattern) f_lasti points at the
        # PRECALL instruction before CALL. In this case point it to the next call function after it's offset
        # (which will be CALL)
        # https://docs.python.org/3.11/library/dis.html#opcode-PRECALL
        last_i = [offset for offset in call_function_locs.keys() if offset > last_i][0]

    output_var_names = []
    last_func_offset = call_function_locs[last_i][0]
    # On Windows since migrating to Python 3.10, the last instruction index appears
    # to be one step behind where it should be. We think it's related to the comment
    # here:
    # https://github.com/python/cpython/blob/v3.8.3/Python/ceval.c#L1139
    _, _, last_i_name, _, _ = ins_stack[last_func_offset]
    next_instruction_offset, _, next_instruction_name, _, _ = ins_stack[last_func_offset + 1]
    if last_i_name == "DICT_MERGE" and next_instruction_name in OPERATOR_NAMES:
        last_func_offset += 1
        last_i = next_instruction_offset

    _, _, name, _, argvalue = ins_stack[last_func_offset + 1]
    if name == "POP_TOP":  # no return values
        pass
    elif name == "STORE_FAST" or name == "STORE_NAME":  # one return value
        output_var_names.append(argvalue)
    elif name == "UNPACK_SEQUENCE":  # Many Return Values, One equal sign
        for index in range(argvalue):
            _, _, _, _, sequence_argvalue = ins_stack[last_func_offset + 2 + index]
            output_var_names.append(sequence_argvalue)
    max_returns = len(output_var_names)
    if name == "COPY":  # Many Return Values, Many equal signs
        # The output here should be a multi-dim list which mimics the variable unpacking sequence.
        # For instance a,b=c,d=f() => [ ['a','b'] , ['c','d'] ]
        #              a,b=c=d=f() => [ ['a','b'] , 'c','d' ]  So on and so forth.

        # put this in a loop and stack the results in an array.
        count = 0
        max_returns = 0  # Must count the max_returns ourselves in this case
        while count < len(ins_stack[call_function_locs[last_i][0] : call_function_locs[last_i][1]]):
            _, _, multi_name, _, multi_argvalue = ins_stack[call_function_locs[last_i][0] + count]
            if multi_name == "UNPACK_SEQUENCE":  # Many Return Values, One equal sign
                hold = []
                if multi_argvalue > max_returns:
                    max_returns = multi_argvalue
                for index in range(multi_argvalue):
                    _, _, _, _, sequence_argvalue = ins_stack[call_function_locs[last_i][0] + count + 1 + index]
                    hold.append(sequence_argvalue)
                count += multi_argvalue
                output_var_names.append(hold)
            # Need to now skip the entries we just appended with the for loop.
            if multi_name == "STORE_FAST" or multi_name == "STORE_NAME":  # One Return Value
                if max_returns == 0:
                    max_returns = 1
                output_var_names.append(multi_argvalue)
            count += 1

    return max_returns, tuple(output_var_names)


# -------------------------------------------------------------------------------


def lhs_info(output_type="both", frame=None):
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

    if output_type == "nreturns":
        return ret_vals[0]
    elif output_type == "names":
        return ret_vals[1]
    else:
        return ret_vals


# -------------------------------------------------------------------------------
