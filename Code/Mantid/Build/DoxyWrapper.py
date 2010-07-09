import sys
import re
import subprocess as sp


process = sp.Popen("doxygen "+sys.argv[1],stderr=sp.PIPE,shell=True)

regExStrings = ['return type of member.*(c_|m_|g_).*is not documented', #remove warnings about return type of member and global variables
'return type of member.*Mantid::PhysicalConstants::.*is not documented',#not return type moans about our constants file please
'Problems running dvips', #moans about dvips not being installed
'Problems running latex'] #moans about latex not being installed

(stdoutString, stdErrString) = process.communicate()

for line in stdErrString:#process.stderr:
	matchCount = 0
	for regExString in regExStrings:
		if (re.search(regExString,line) != None):
			matchCount+=1
			break
	if (matchCount == 0):
		print >> sys.stderr, line.rstrip() 

