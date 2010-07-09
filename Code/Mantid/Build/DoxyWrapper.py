import sys
import re
import subprocess as sp


process = sp.Popen("doxygen "+sys.argv[1],stderr=sp.PIPE,shell=True)

regExStrings = ['return type of member.*(m_|g_).*is not documented', #remove warnings about return type of member and global variables
'Problems running dvips', #moans about dvips not being installed
'Problems running latex'] #moans about latex not being installed

for line in process.stderr:
	matchCount = 0
	for regExString in regExStrings:
		if (re.search(regExString,line) != None):
			matchCount+=1
	if (matchCount == 0):
		print >> sys.stderr, line.rstrip() 

