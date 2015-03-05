''' SVN Info:  	The variables below will only get subsituted at svn checkout if
		the repository is configured for variable subsitution. 

	$Id$
	$HeadURL$
|=============================================================================|=======|	
1                                                                            80   <tab>
'''
import sys, os
import dis, inspect, opcode
def ls():
	print os.getcwd()	
	files=os.listdir(os.getcwd())
	for i in range(0,len(files)):

                print files[i]
def pwd():
	print os.getcwd()
def cd(dir_str):
	os.chdir(dir_str)
def lineno():
    """
    call signature(s)::
    lineno()

    Returns the current line number in our program.
    
    No Arguments.


    Working example
    >>> print "This is the line number ",lineno(),"\n"
    
    """
    return inspect.currentframe().f_back.f_lineno

def decompile(code_object):
	''' 	taken from http://thermalnoise.wordpress.com/2007/12/30/exploring-python-bytecode/

	decompile extracts dissasembly information from the byte code and stores it in a 
		list for further use.
	
	call signature(s)::
		instructions=decompile(f.f_code)

	Required arguments:
	=========   =====================================================================
	f.f_code    A  bytecode object ectracted with inspect.currentframe()
		    or anyother mechanism that returns byte code.   
	
	Optional keyword arguments: NONE	

	Outputs:
	=========   =====================================================================
	instructions  a list of offsets, op_codes, names, arguments, argument_type, 
			argument_value which can be deconstructed to find out various things
			about a function call.

	Examples:
	
	f = inspect.currentframe().f_back.f_back
	i = f.f_lasti  # index of the last attempted instruction in byte code
	ins=decompile(f.f_code)
	pretty_print(ins)
	

	'''
	code = code_object.co_code
	variables = code_object.co_cellvars + code_object.co_freevars
	instructions = []
	n = len(code)
	i = 0
	e = 0
	while i < n:
		i_offset = i
		i_opcode = ord(code[i])
		i = i + 1
		if i_opcode >= opcode.HAVE_ARGUMENT:
			i_argument = ord(code[i]) + (ord(code[i+1]) << (4*2)) + e
			i = i +2
			if i_opcode == opcode.EXTENDED_ARG:
				e = iarg << 16
			else:
				e = 0
			if i_opcode in opcode.hasconst:
				i_arg_value = repr(code_object.co_consts[i_argument])
				i_arg_type = 'CONSTANT'
			elif i_opcode in opcode.hasname:
				i_arg_value = code_object.co_names[i_argument]
				i_arg_type = 'GLOBAL VARIABLE'
			elif i_opcode in opcode.hasjrel:
				i_arg_value = repr(i + i_argument)
				i_arg_type = 'RELATIVE JUMP'
			elif i_opcode in opcode.haslocal:
				i_arg_value = code_object.co_varnames[i_argument]
				i_arg_type = 'LOCAL VARIABLE'
			elif i_opcode in opcode.hascompare:
				i_arg_value = opcode.cmp_op[i_argument]
				i_arg_type = 'COMPARE OPERATOR'
			elif i_opcode in opcode.hasfree:
				i_arg_value = variables[i_argument]
				i_arg_type = 'FREE VARIABLE'
			else:
				i_arg_value = i_argument
				i_arg_type = 'OTHER'
		else:
			i_argument = None
			i_arg_value = None
			i_arg_type = None
		instructions.append( (i_offset, i_opcode, opcode.opname[i_opcode], i_argument, i_arg_type, i_arg_value) )
	return instructions
 
# Print the byte code in a human readable format
def pretty_print(instructions):
	print '%5s %-20s %3s  %5s  %-20s  %s' %  ('OFFSET', 'INSTRUCTION', 'OPCODE', 'ARG', 'TYPE', 'VALUE')
	for (offset, op, name, argument, argtype, argvalue) in instructions:
		print '%5d  %-20s (%3d)  ' % (offset, name, op),
		if argument != None:
			print '%5d  %-20s  (%s)' % (argument, argtype, argvalue),
		print

def expecting():
	#{{{
	'''
	call signature(s)::


	Return how many values the caller is expecting

	Required arguments:	NONE
	
	Optional keyword arguments: NONE	


	Outputs:
	=========   =====================================================================
	numReturns	Number of return values on expected on the left of the equal sign.

	Examples:

	This function is not designed for cammand line use.  Using in a function can 
	follow the form below.
	

	def test1():
		def f():
			r = expecting()
			print r
			if r == 0:
				return None
			if r == 1:
				return 0
			return range(r)

		f()
		print "---"
		a = f()
		print "---", a
		a, b = f()
		print "---", a,b
		a, b = c = f()
		print "---", a,b,c
		a, b = c = d = f()
		print "---", a,b,c
		a = b = f()
		print "---", a,b
		a = b, c = f()
		print "---", a,b,c
		a = b = c, d = f()
		print "---", a,b,c,d
		a = b, c = d = f()
		print "---", a,b,c,d
		a, b = c, d = f()
		print "---", a,b,c,d
	'''
	#}}}

	""" Developers Notes:
		
		Now works with an multiple assigments correctly.  This is verified by 
		test() and test1() below
	"""
	f = inspect.currentframe().f_back.f_back
	i = f.f_lasti  # index of the last attempted instruction in byte code
	ins=decompile(f.f_code)
	#pretty_print(ins)
	for (offset, op, name, argument, argtype, argvalue) in ins:
		if offset > i:
			if name == 'POP_TOP':
				return 0
			if name == 'UNPACK_SEQUENCE':
				return argument
			if name == 'CALL_FUNCTION':
				return 1

def lhs(output='names'):
	''' 	
	call signature(s)::

	Return how many values the caller is expecting

	Required arguments:	NONE
	
	Optional keyword arguments: NONE	


	Outputs:
	=========   =====================================================================
	numReturns	Number of return values on expected on the left of the equal sign.

	Examples:

	This function is not designed for cammand line use.  Using in a function can 
	follow the form below.

	'''
	""" Developers Notes:
	"""
	f = inspect.currentframe().f_back.f_back
	i = f.f_lasti  # index of the last attempted instruction in byte code
	ins=decompile(f.f_code)
	#pretty_print(ins)

	CallFunctionLocation={}
	first=False; StartIndex=0; StartOffset=0
	# we must list all of the operators that behave like a function call in byte-code
	OperatorNames=set(['CALL_FUNCTION','UNARY_POSITIVE','UNARY_NEGATIVE','UNARY_NOT','UNARY_CONVERT','UNARY_INVERT','GET_ITER', 'BINARY_POWER','BINARY_MULTIPLY','BINARY_DIVIDE', 'BINARY_FLOOR_DIVIDE', 'BINARY_TRUE_DIVIDE', 'BINARY_MODULO','BINARY_ADD','BINARY_SUBTRACT','BINARY_SUBSCR','BINARY_LSHIFT','BINARY_RSHIFT','BINARY_AND','BINARY_XOR','BINARY_OR'])

	for index in range(len(ins)):
		(offset, op, name, argument, argtype, argvalue) = ins[index]
		if name in OperatorNames:
			if not first:
				CallFunctionLocation[StartOffset] = (StartIndex,index)
			StartIndex=index
			StartOffset = offset

	(offset, op, name, argument, argtype, argvalue) = ins[-1]
	CallFunctionLocation[StartOffset]=(StartIndex,len(ins)-1) # append the index of the last entry to form the last boundary

	#print CallFunctionLocation
	#pretty_print( ins[CallFunctionLocation[i][0]:CallFunctionLocation[i][1]] )
	# In our case i should always be the offset of a Call_Function instruction. We can use this to baracket 
	# the bit which we are interested in
	
	OutputVariableNames=[]
	(offset, op, name, argument, argtype, argvalue) = ins[CallFunctionLocation[i][0] + 1] 
	if name == 'POP_TOP':  # no Return Values
		pass
		#return OutputVariableNames
	if name == 'STORE_FAST' or name == 'STORE_NAME': # One Return Value 
		OutputVariableNames.append(argvalue)
	if name == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign 
		for index in range(argvalue):
			(offset_, op_, name_, argument_, argtype_, argvalue_) = ins[CallFunctionLocation[i][0] + 1 + 1 +index] 
			OutputVariableNames.append(argvalue_)
	maxReturns = len(OutputVariableNames)
	if name == 'DUP_TOP': # Many Return Values, Many equal signs
		# The output here should be a multi-dim list which mimics the variable unpacking sequence.
		# For instance a,b=c,d=f() => [ ['a','b'] , ['c','d'] ]
		#              a,b=c=d=f() => [ ['a','b'] , 'c','d' ]  So on and so forth.

		# put this in a loop and stack the results in an array.
		count = 0; maxReturns = 0 # Must count the maxReturns ourselves in this case
		while count < len(ins[CallFunctionLocation[i][0] :CallFunctionLocation[i][1]]):
			(offset_, op_, name_, argument_, argtype_, argvalue_) = ins[CallFunctionLocation[i][0]+count] 
			#print 'i= ',i,'count = ', count, 'maxReturns = ',maxReturns
			if name_ == 'UNPACK_SEQUENCE': # Many Return Values, One equal sign 
				hold=[]
				#print 'argvalue_ = ', argvalue_, 'count = ',count
				if argvalue_ > maxReturns:
					maxReturns=argvalue_
				for index in range(argvalue_):
					(_offset_, _op_, _name_, _argument_, _argtype_, _argvalue_) = ins[CallFunctionLocation[i][0] + count+1+index] 
					hold.append(_argvalue_)
				count = count + argvalue_
				OutputVariableNames.append(hold)
			# Need to now skip the entries we just appended with the for loop.
			if name_ == 'STORE_FAST' or name_ == 'STORE_NAME': # One Return Value 
				if 1 > maxReturns:
					maxReturns = 1
				OutputVariableNames.append(argvalue_)
			count = count + 1


	# Now that OutputVariableNames is filled with the right stuff we need to output the correct thing. Either the maximum number of 
	# variables to unpack in the case of multiple ='s or just the length of the array or just the naames of the variables.		
	
	if output== 'names':
		return OutputVariableNames
	elif output == 'number':
		return maxReturns 
	elif output == 'both':
		return (maxReturns,OutputVariableNames)
	
	return 0 # Should never get to here

